#include "ObstacleFetcherComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "HttpModule.h"               // Добавьте
#include "Interfaces/IHttpRequest.h"  // Добавьте
#include "Interfaces/IHttpResponse.h" // Добавьте
#include "Json.h"                     // Добавьте
#include "JsonUtilities.h"            // Добавьте
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UObstacleFetcherComponent::UObstacleFetcherComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    BBoxString = TEXT("55.9026388890000021,37.4043055559999971,56.4468055560000010,38.4101388890000024");
}

void UObstacleFetcherComponent::BeginPlay()
{
    Super::BeginPlay();

    // Стартуем FetchObstacles через 5 секунд, потом циклически
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UObstacleFetcherComponent::FetchObstacles,
                                           10.0f, // интервал
                                           true,  // повторять
                                           5.0f   // первая задержка
    );

    // Запускаем таймер обновления токена каждые 90 секунд
    GetWorld()->GetTimerManager().SetTimer(TokenRefreshHandle, this, &UObstacleFetcherComponent::TokenRefresh,
                                           90.0f, // интервал
                                           true,  // повторять
                                           90.0f  // первая задержка
    );
}

void UObstacleFetcherComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    if (EndPlayReason == EEndPlayReason::EndPlayInEditor)
    {
        Logout();
    }
}

void UObstacleFetcherComponent::RegisterUser()
{
    if (Login.IsEmpty() || Password.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Login or Password is empty"));
        RegistrationLog = FText::FromString(TEXT("Login or Password is empty"));
        return;
    }

    TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
    RequestObj->SetStringField("username", Login);
    RequestObj->SetStringField("password", Password);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://localhost:8000/api/register/"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(OutputString);

    // Захватываем this, чтобы иметь доступ к RegistrationLog
    Request->OnProcessRequestComplete().BindLambda(
        [this](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
        {
            if (bSuccess && (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201))
            {
                FString Message = FString::Printf(TEXT("Успешная регистрация!"), *Login, *Password);
                RegistrationLog = FText::FromString(Message);
                UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
            }
            else
            {
                FString ErrorMsg = FString::Printf(TEXT("Ошибка регистрации."), *Response->GetContentAsString());
                RegistrationLog = FText::FromString(ErrorMsg);
                UE_LOG(LogTemp, Error, TEXT("%s"), *ErrorMsg);
            }
        });

    Request->ProcessRequest();
}

void UObstacleFetcherComponent::LoginUser()
{
    if (Login.IsEmpty() || Password.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("Login or Password is empty"));
        return;
    }

    TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
    RequestObj->SetStringField("username", Login);
    RequestObj->SetStringField("password", Password);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://localhost:8000/api/token/"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(OutputString);
    Request->OnProcessRequestComplete().BindUObject(this, &UObstacleFetcherComponent::OnLoginResponse);
    Request->ProcessRequest();
}

void UObstacleFetcherComponent::OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (bWasSuccessful && (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 201))
    {
        FString ResponseString = Response->GetContentAsString();
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

        if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
        {
            AuthToken = JsonObject->GetStringField("access");
            RefreshToken = JsonObject->GetStringField("refresh"); // <-- добавлено

            UE_LOG(LogTemp, Warning, TEXT("Login successful."));
            UE_LOG(LogTemp, Warning, TEXT("Access Token: %s"), *AuthToken);
            UE_LOG(LogTemp, Warning, TEXT("Refresh Token: %s"), *RefreshToken);

            AuthorizeLog = FText::FromString(TEXT("Успешная авторизация!"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to parse login response JSON"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Login failed: %s"), *Response->GetContentAsString());
        AuthorizeLog = FText::FromString(TEXT("Ошибка авторизации."));
    }
}

void UObstacleFetcherComponent::TokenRefresh()
{
    if (AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("AuthToken is empty. Skipping token refresh."));
        return;
    }

    if (RefreshToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("RefreshToken is empty!"));
        return;
    }

    TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
    RequestObj->SetStringField("refresh", RefreshToken); // Тело запроса

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer);

    // Только теперь логируем
    UE_LOG(LogTemp, Warning, TEXT("Sending refresh token: %s"), *RefreshToken);
    UE_LOG(LogTemp, Warning, TEXT("Request body: %s"), *OutputString);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://localhost:8000/api/token/refresh/"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetContentAsString(OutputString);
    Request->OnProcessRequestComplete().BindLambda(
        [this](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
        {
            if (Response.IsValid())
            {
                UE_LOG(LogTemp, Error, TEXT("Response Code: %d"), Response->GetResponseCode());
                UE_LOG(LogTemp, Error, TEXT("Response Content: %s"), *Response->GetContentAsString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Response invalid or null."));
            }

            if (bSuccess && Response->GetResponseCode() == 200)
            {
                FString ResponseString = Response->GetContentAsString();
                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
                if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
                {
                    FString NewAccessToken = JsonObject->GetStringField("access");
                    AuthToken = NewAccessToken;
                    UE_LOG(LogTemp, Warning, TEXT("Token refreshed successfully. New token: %s"), *AuthToken);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to refresh token."));
            }
        });

    Request->ProcessRequest();
}

void UObstacleFetcherComponent::Logout()
{
    if (RefreshToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("RefreshToken is empty! Cannot logout."));
        return;
    }

    TSharedPtr<FJsonObject> RequestObj = MakeShareable(new FJsonObject);
    RequestObj->SetStringField("refresh", RefreshToken);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RequestObj.ToSharedRef(), Writer);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(TEXT("http://localhost:8000/api/logout/"));
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");

    if (!AuthToken.IsEmpty())
    {
        FString AuthHeader = FString::Printf(TEXT("Bearer %s"), *AuthToken);
        Request->SetHeader("Authorization", AuthHeader);
    }

    Request->SetContentAsString(OutputString);

    Request->OnProcessRequestComplete().BindLambda(
        [this](FHttpRequestPtr, FHttpResponsePtr Response, bool bSuccess)
        {
            if (bSuccess && Response->GetResponseCode() == 205)
            {
                UE_LOG(LogTemp, Warning, TEXT("Logout successful"));
                AuthToken.Empty();
                RefreshToken.Empty();
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Logout failed: %s"),
                       Response.IsValid() ? *Response->GetContentAsString() : TEXT("No response"));
            }
        });

    Request->ProcessRequest();
}

void UObstacleFetcherComponent::TryConnectToServer()
{
    // Используем ServerURL вместо TestURL
    FString FinalURL = ServerURL.IsEmpty() ? TEXT("http://localhost:8000") : ServerURL;

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FinalURL);
    Request->SetVerb("GET");
    Request->OnProcessRequestComplete().BindUObject(this, &UObstacleFetcherComponent::OnServerConnectionResponse);
    Request->ProcessRequest();
}

void UObstacleFetcherComponent::OnServerConnectionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response,
                                                           bool bWasSuccessful)
{
    if (bWasSuccessful && Response.IsValid() && Response->GetResponseCode() == 200)
    {
        ConnectServerLog = FText::FromString(TEXT("Подключено к серверу!"));
        ServerURL = TEXT("http://localhost:8000/api/points/");
    }
    else
    {
        ConnectServerLog = FText::FromString(TEXT("Ошибка подключения!"));
        // ServerURL не меняется
    }

    // Если хотите лог в консоль:
    UE_LOG(LogTemp, Log, TEXT("ConnectServerLog: %s"), *ConnectServerLog.ToString());
}

void UObstacleFetcherComponent::FetchObstacles()
{
    if (ServerURL.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ServerURL is empty!"));
        return;
    }

    FVector Location;

    if (bUseStaticLocation)
    {
        if (!BBoxString.IsEmpty())
        {
            TArray<FString> BBoxParts;
            BBoxString.ParseIntoArray(BBoxParts, TEXT(","), true);

            if (BBoxParts.Num() == 4)
            {
                double MinLat = FCString::Atod(*BBoxParts[0]);
                double MinLon = FCString::Atod(*BBoxParts[1]);
                double MaxLat = FCString::Atod(*BBoxParts[2]);
                double MaxLon = FCString::Atod(*BBoxParts[3]);

                double CenterLat = (MinLat + MaxLat) / 2.0;
                double CenterLon = (MinLon + MaxLon) / 2.0;

                Location = FVector((float)CenterLon, (float)CenterLat, 0.0f);
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("BBoxString format invalid, expected 4 comma-separated values."));
                // fallback на фиксированную точку, если парсинг не удался
                Location = FVector(37.9072222225f, 56.1747222225f, 0.0f);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("BBoxString is empty, cannot compute static location."));
            Location = FVector(37.9072222225f, 56.1747222225f, 0.0f);
        }
    }
    else
    {
        if (bUseCameraLocation)
        {
            if (APlayerController *PC = GetWorld()->GetFirstPlayerController())
            {
                Location = PC->PlayerCameraManager->GetCameraLocation();
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("No PlayerController found!"));
                return;
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
    }

    glm::dvec3 LongLatHeight;

    if (bUseStaticLocation)
    {
        // Для статической локации координаты берем как есть (X=Longitude, Y=Latitude, Z=0)
        LongLatHeight = glm::dvec3(Location.X, Location.Y, Location.Z);
    }
    else
    {
        if (!Georeference)
        {
            UE_LOG(LogTemp, Warning, TEXT("Georeference not assigned!"));
            return;
        }

        LongLatHeight =
            Georeference->TransformUeToLongitudeLatitudeHeight(glm::dvec3(Location.X, Location.Y, Location.Z));
    }

    FString FinalURL =
        FString::Printf(TEXT("%s?point=%f,%f,%f"), *ServerURL, LongLatHeight.x, LongLatHeight.y, LongLatHeight.z);

    if (!BBoxString.IsEmpty())
    {
        FinalURL += FString::Printf(TEXT("&bbox=%s"), *BBoxString);
    }

    if (!TypeFilter.IsEmpty())
    {
        FinalURL += FString::Printf(TEXT("&type=%s"), *FGenericPlatformHttp::UrlEncode(TypeFilter));
    }

    if (Elevate != 0)
    {
        FinalURL += FString::Printf(TEXT("&elev=%d"), Elevate);
    }

    if (Limit != 0)
    {
        FinalURL += FString::Printf(TEXT("&limit=%d"), Limit);
    }

    if (SearchRadius != 0.0f)
    {
        FinalURL += FString::Printf(TEXT("&radius=%f"), SearchRadius);
    }

    UE_LOG(LogTemp, Warning, TEXT("Requesting obstacles with URL: %s"), *FinalURL);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(FinalURL);
    Request->SetVerb("GET");
    Request->OnProcessRequestComplete().BindUObject(this, &UObstacleFetcherComponent::OnResponseReceived);

    if (!AuthToken.IsEmpty())
    {
        Request->SetHeader("Authorization", FString::Printf(TEXT("Bearer %s"), *AuthToken));
    }

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
        Obstacle.Type = ObstacleObj->GetStringField("type");
        Obstacle.Distance = ObstacleObj->GetNumberField("distance");

        if (ObstacleObj->HasField("geom"))
        {
            Obstacle.Geom = ObstacleObj->GetStringField("geom");
            double Lon, Lat, Elev;
            if (ParseGeomWKT(Obstacle.Geom, Lon, Lat, Elev))
            {
                Obstacle.ParsedLon = Lon;
                Obstacle.ParsedLat = Lat;
                Obstacle.ParsedElev = Elev;
                Obstacle.bUseGeom = true;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to parse geom: %s"), *Obstacle.Geom);
            }
        }
        else
        {
            Obstacle.Longitude = ObstacleObj->GetNumberField("lon");
            Obstacle.Latitude = ObstacleObj->GetNumberField("lat");
            Obstacle.Elevation = ObstacleObj->GetNumberField("elev");
        }

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

                // Удаляем текст из мапы и уничтожаем компонент
                if (SpawnedTextComponents.Contains(ActorGuid))
                {
                    UTextRenderComponent *TextComp = SpawnedTextComponents[ActorGuid];
                    if (TextComp)
                    {
                        TextComp->DestroyComponent();
                    }
                    SpawnedTextComponents.Remove(ActorGuid);
                }

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

bool UObstacleFetcherComponent::ParseGeomWKT(const FString &GeomString, double &OutLon, double &OutLat, double &OutElev)
{
    FString CleanString = GeomString;
    CleanString = CleanString.Replace(TEXT("SRID=4326;"), TEXT(""));
    CleanString = CleanString.Replace(TEXT("POINT Z ("), TEXT(""));
    CleanString = CleanString.Replace(TEXT(")"), TEXT(""));

    TArray<FString> Parts;
    CleanString.ParseIntoArray(Parts, TEXT(" "), true);

    if (Parts.Num() == 3)
    {
        OutLon = FCString::Atod(*Parts[0]);
        OutLat = FCString::Atod(*Parts[1]);
        OutElev = FCString::Atod(*Parts[2]);
        return true;
    }

    return false;
}

void UObstacleFetcherComponent::SpawnObstacle(const FObstacleData &Data)
{
    if (!Georeference)
        return;

    if (SpawnedObstacleGUIDs.Contains(Data.Guid))
    {
        UE_LOG(LogTemp, Warning, TEXT("Obstacle with GUID %s already exists. Skipping spawn."), *Data.Guid);
        UpdateObstacleText(Data);
        return;
    }

    glm::dvec3 UECoords;

    if (Data.bUseGeom)
    {
        UECoords = Georeference->TransformLongitudeLatitudeHeightToUe(
            glm::dvec3(Data.ParsedLon, Data.ParsedLat, Data.ParsedElev));
    }
    else
    {
        UECoords = Georeference->TransformLongitudeLatitudeHeightToUe(
            glm::dvec3(Data.Longitude, Data.Latitude, Data.Elevation));
    }

    float MinZ = 10000.0f;
    float MaxZ = 90000.0f;
    float MaxElevation = 800.0f;

    float ElevValue = Data.bUseGeom ? Data.ParsedElev : Data.Elevation;
    float ScaledZ = MinZ + (ElevValue / MaxElevation) * (MaxZ - MinZ);

    FVector Location(UECoords.x, UECoords.y, ScaledZ);

    UE_LOG(LogTemp, Warning, TEXT("Spawning Obstacle:"));
    UE_LOG(LogTemp, Warning, TEXT("  GUID: %s"), *Data.Guid);
    UE_LOG(LogTemp, Warning, TEXT("  Type: %s"), *Data.Type);
    UE_LOG(LogTemp, Warning, TEXT("  Longitude: %f"), Data.Longitude);
    UE_LOG(LogTemp, Warning, TEXT("  Latitude: %f"), Data.Latitude);
    UE_LOG(LogTemp, Warning, TEXT("  Elevation: %f"), Data.Elevation);
    UE_LOG(LogTemp, Warning, TEXT("  Distance: %f"), Data.Distance);
    UE_LOG(LogTemp, Warning, TEXT("  World Location: X=%f Y=%f Z=%f"), Location.X, Location.Y, Location.Z);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

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

            SpawnedActor->SetActorLabel(Data.Guid);

            UStaticMeshComponent *MeshComponent = SpawnedActor->GetStaticMeshComponent();
            if (MeshComponent)
            {
                MeshComponent->SetMobility(EComponentMobility::Movable);
                MeshComponent->SetWorldScale3D(FVector(50.0f, 50.0f, 50.0f));
                MeshComponent->SetStaticMesh(MeshToUse);
                MeshComponent->SetMaterial(0, MaterialToUse);

                // Добавляем TextRenderComponent для отображения Distance
                UTextRenderComponent *TextComponent = NewObject<UTextRenderComponent>(SpawnedActor);
                if (TextComponent)
                {
                    TextComponent->RegisterComponent();
                    TextComponent->AttachToComponent(SpawnedActor->GetRootComponent(),
                                                     FAttachmentTransformRules::KeepRelativeTransform);
                    TextComponent->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
                    TextComponent->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), Data.Distance)));
                    TextComponent->SetHorizontalAlignment(EHTA_Center);
                    TextComponent->SetWorldSize(200.f);
                    TextComponent->SetTextRenderColor(FColor::Red);

                    // Сохраняем указатель в мапу по GUID
                    SpawnedTextComponents.Add(Data.Guid, TextComponent);
                }

                SpawnedObstacles.Add(SpawnedActor);
                SpawnedObstacleGUIDs.Add(Data.Guid);
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to load mesh!"));
    }
}

void UObstacleFetcherComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                              FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetWorld())
        return;

    APlayerCameraManager *CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
    if (!CameraManager)
        return;

    FVector CameraLocation = CameraManager->GetCameraLocation();

    for (auto &Pair : SpawnedTextComponents)
    {
        UTextRenderComponent *TextComp = Pair.Value;
        if (TextComp)
        {
            FVector TextLocation = TextComp->GetComponentLocation();
            FVector Direction = (CameraLocation - TextLocation).GetSafeNormal();
            FRotator LookAtRotation = Direction.Rotation();

            // Чтобы текст не был вверх ногами, можно сбросить крен:
            LookAtRotation.Roll = 0;

            TextComp->SetWorldRotation(LookAtRotation);
        }
    }
}

void UObstacleFetcherComponent::UpdateObstacleText(const FObstacleData &Data)
{
    if (UTextRenderComponent **TextCompPtr = SpawnedTextComponents.Find(Data.Guid))
    {
        UTextRenderComponent *TextComp = *TextCompPtr;
        if (TextComp)
        {
            TextComp->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), Data.Distance)));
        }
    }
}