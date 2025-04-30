#include "ObstacleFetcherComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "HttpModule.h"               
#include "Interfaces/IHttpRequest.h"  
#include "Interfaces/IHttpResponse.h"
#include "Json.h"                     
#include "JsonUtilities.h"           
#include "TimerManager.h"

UObstacleFetcherComponent::UObstacleFetcherComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UObstacleFetcherComponent::BeginPlay()
{
    Super::BeginPlay();
    FetchObstacles(); // Выполняем запрос сразу при старте
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UObstacleFetcherComponent::FetchObstacles, 5.0f,
                                           true); // Далее повторяем каждые 5 секунд
}

void UObstacleFetcherComponent::FetchObstacles()
{
    if (ServerURL.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ServerURL is empty!"));
        return;
    }

    FVector Location;

    if (bUseCameraLocation)
    {
        if (APlayerController *PC = GetWorld()->GetFirstPlayerController())
        {
            Location = PC->PlayerCameraManager->GetCameraLocation();
        }
    }
    else if (TrackedActor)
    {
        Location = TrackedActor->GetActorLocation();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid camera or tracked actor!"));
        return;
    }

    if (!Georeference)
    {
        UE_LOG(LogTemp, Warning, TEXT("Georeference not assigned!"));
        return;
    }

    glm::dvec3 LongLatHeight =
        Georeference->TransformUeToLongitudeLatitudeHeight(glm::dvec3(Location.X, Location.Y, Location.Z));

    FString FinalURL = FString::Printf(TEXT("%s?point=%f,%f&radius=10"), *ServerURL, LongLatHeight.x, LongLatHeight.y);

    // Исправленная строка:
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FinalURL);
    Request->SetVerb("GET");
    Request->OnProcessRequestComplete().BindUObject(this, &UObstacleFetcherComponent::OnResponseReceived);
    Request->ProcessRequest();
}

void UObstacleFetcherComponent::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                   bool bWasSuccessful)
{
    if (!bWasSuccessful || Response->GetResponseCode() != 200)
    {
        UE_LOG(LogTemp, Error, TEXT("Request failed with code: %d"), Response->GetResponseCode());
        return;
    }

    FString ResponseString = Response->GetContentAsString();
    TArray<TSharedPtr<FJsonValue>> JsonArray;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

    if (!FJsonSerializer::Deserialize(Reader, JsonArray))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON response"));
        return;
    }

    // Преобразуем TArray<TSharedPtr<FJsonValue>> в TArray<FObstacleData>
    TArray<FObstacleData> ParsedObstacles;
    for (const TSharedPtr<FJsonValue> &Value : JsonArray)
    {
        const TSharedPtr<FJsonObject> &ObstacleObj = Value->AsObject();
        if (!ObstacleObj.IsValid())
            continue;

        FObstacleData Obstacle;
        Obstacle.Guid = ObstacleObj->GetStringField("guid");
        Obstacle.Longitude = ObstacleObj->GetNumberField("lon");
        Obstacle.Latitude = ObstacleObj->GetNumberField("lat");
        Obstacle.Elevation = ObstacleObj->GetNumberField("elev");
        Obstacle.Type = ObstacleObj->GetStringField("type");

        ParsedObstacles.Add(Obstacle);
    }

    // Теперь передаем уже преобразованный массив в функцию ClearObstacles
    ClearObstacles(ParsedObstacles);

    // Добавление новых объектов
    for (const FObstacleData &Obstacle : ParsedObstacles)
    {
        SpawnObstacle(Obstacle);
    }
}


void UObstacleFetcherComponent::ClearObstacles(const TArray<FObstacleData> &NewObstacles)
{
    // Массив для хранения новых GUID объектов
    TArray<FString> NewObstacleGUIDs;
    for (const FObstacleData &NewObstacle : NewObstacles)
    {
        NewObstacleGUIDs.Add(NewObstacle.Guid);
    }

    // Удаление объектов, которых больше нет в новых данных
    for (int32 i = SpawnedObstacles.Num() - 1; i >= 0; --i)
    {
        AActor *ActorToRemove = SpawnedObstacles[i];
        FString ActorGuid = SpawnedObstacleGUIDs[i]; // Получаем GUID объекта

        // Если объект не был уничтожен
        if (ActorToRemove && !ActorToRemove->IsPendingKill())
        {
            bool bFound = false;
            // Проверяем, существует ли объект с таким GUID в новых данных
            for (const FString &NewGuid : NewObstacleGUIDs)
            {
                if (NewGuid == ActorGuid)
                {
                    bFound = true;
                    break;
                }
            }

            // Если объекта с таким GUID нет в новых данных, удаляем его
            if (!bFound)
            {
                UE_LOG(LogTemp, Warning, TEXT("Destroying obstacle with GUID: %s"), *ActorGuid);
                ActorToRemove->Destroy();
                SpawnedObstacles.RemoveAt(i);
                SpawnedObstacleGUIDs.RemoveAt(i);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Found obstacle with GUID: %s, not destroying."), *ActorGuid);
            }
        }
    }
}

void UObstacleFetcherComponent::SpawnObstacle(const FObstacleData &Data)
{
    if (!Georeference)
        return;

    // Проверяем, не существует ли уже объект с таким же GUID
    if (SpawnedObstacleGUIDs.Contains(Data.Guid))
    {
        UE_LOG(LogTemp, Warning, TEXT("Obstacle with GUID %s already exists. Skipping spawn."), *Data.Guid);
        return;
    }

    // Продолжаем спавнить объект, если его еще нет
    glm::dvec3 UECoords =
        Georeference->TransformLongitudeLatitudeHeightToUe(glm::dvec3(Data.Longitude, Data.Latitude, Data.Elevation));

    FVector Location(UECoords.x, UECoords.y, 10000.0f); // Высота для всех объектов 10000.0f

    UE_LOG(LogTemp, Warning, TEXT("Spawning at World Location: X=%f Y=%f Z=%f"), Location.X, Location.Y, Location.Z);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // Загружаем соответствующую модель в зависимости от типа объекта
    UStaticMesh *MeshToUse = nullptr;
    UMaterialInterface *MaterialToUse = nullptr;

    if (Data.Type == "COMMUNICATION_TOWER")
    {
        MeshToUse = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AirplaneLogic/Objects/Materials/Dote.Dote"));
        MaterialToUse =
            LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/AirplaneLogic/Objects/Materials/DoteObj.DoteObj"));
    }
    else if (Data.Type == "CHIMNEY")
    {
        MeshToUse = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AirplaneLogic/Objects/Materials/LineObj.LineObj"));
        MaterialToUse = LoadObject<UMaterialInterface>(
            nullptr, TEXT("/Game/AirplaneLogic/Objects/Materials/LineObject.LineObject"));
    }
    else if (Data.Type == "ANTENNA")
    {
        MeshToUse = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AirplaneLogic/Objects/Materials/AreaObj.AreaObj"));
        MaterialToUse =
            LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/AirplaneLogic/Objects/Materials/Area.Area"));
    }
    else
    {
        MeshToUse = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AirplaneLogic/Objects/Materials/AirTrace.AirTrace"));
        MaterialToUse =
            LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/AirplaneLogic/Objects/Materials/AirTr.AirTr"));
    }

    if (MeshToUse)
    {
        AStaticMeshActor *SpawnedActor =
            GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator, SpawnParams);
        if (SpawnedActor)
        {
            UStaticMeshComponent *MeshComponent = SpawnedActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                MeshComponent->SetMobility(EComponentMobility::Movable); // Устанавливаем мобильность
                MeshComponent->SetWorldScale3D(FVector(50.0f, 50.0f, 50.0f));
                MeshComponent->SetStaticMesh(MeshToUse);      // Применяем новую сетку
                MeshComponent->SetMaterial(0, MaterialToUse); // Применяем материал
                SpawnedObstacles.Add(SpawnedActor);           // Добавляем в отслеживаемые
                SpawnedObstacleGUIDs.Add(Data.Guid);          // Добавляем GUID объекта
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load mesh!"));
    }
}
