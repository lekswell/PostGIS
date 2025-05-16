#pragma once

#include "CesiumGeoreference.h"
#include "Components/ActorComponent.h"
#include "Components/TextRenderComponent.h"
#include "Containers/Map.h"
#include "CoreMinimal.h"
#include "Http.h"
#include "Kismet/GameplayStatics.h"
#include "ObstacleFetcherComponent.generated.h"

USTRUCT(BlueprintType)
struct FObstacleData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString Guid;

    UPROPERTY(BlueprintReadWrite)
    float Longitude;

    UPROPERTY(BlueprintReadWrite)
    float Latitude;

    UPROPERTY(BlueprintReadWrite)
    float Elevation;

    UPROPERTY(BlueprintReadWrite)
    float Distance;

    UPROPERTY(BlueprintReadWrite)
    FString Type;

    UPROPERTY(BlueprintReadWrite)
    FString Geom; // WKT-строка

    bool bUseGeom = false; // индикатор, что геометрию нужно брать из Geom

    // Для хранения распарсенной координаты
    double ParsedLon = 0.0;
    double ParsedLat = 0.0;
    double ParsedElev = 0.0;
};

// Делегат для обработки ответа сервера

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObstaclesReceived, const TArray<FObstacleData> &, Obstacles);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JET_API UObstacleFetcherComponent : public UActorComponent
{
    GENERATED_BODY()

  public:
    UObstacleFetcherComponent();

    UFUNCTION(BlueprintCallable, Category = "Obstacle Fetcher")
    void FetchObstacles();

    // Делегаты для обработки серверных ответов

    UPROPERTY(BlueprintAssignable, Category = "Obstacle Fetcher")
    FOnObstaclesReceived OnObstaclesReceived;

    UFUNCTION(BlueprintCallable, Category = "Obstacle Auth")
    void RegisterUser();

    UFUNCTION(BlueprintCallable, Category = "Obstacle Auth")
    void LoginUser();

    // Метод для подключения к серверу
    UFUNCTION(BlueprintCallable, Category = "Server")
    void TryConnectToServer();

    // Blueprint Callable строки с логами
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Server")
    FText ConnectServerLog;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Auth")
    FText RegistrationLog;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authorization")
    FText AuthorizeLog;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authorization")
    FString AuthToken;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Authorization")
    FString RefreshToken;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Fetcher")
    bool bUseCameraLocation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Fetcher")
    AActor *TrackedActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Fetcher")
    FString ServerURL;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Filter")
    FString TypeFilter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Filter")
    FString BBoxString;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Filter")
    int32 Elevate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Filter")
    int32 Limit;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Filter")
    float SearchRadius;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Fetcher")
    bool bUseStaticLocation = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Fetcher")
    ACesiumGeoreference *Georeference;

    // Данные для логина и пароля
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Auth")
    FString Login;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Auth")
    FString Password;

  protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    TMap<FString, UTextRenderComponent *> SpawnedTextComponents;
    virtual void TickComponent(float DeltaTime, enum ELevelTick TickType,
                               FActorComponentTickFunction *ThisTickFunction) override;

  private:
    TArray<FString> SpawnedObstacleGUIDs;

    void OnServerConnectionResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void ClearObstacles(const TArray<FObstacleData> &NewObstacles);
    void TokenRefresh();
    void Logout();
    void UpdateObstacleText(const FObstacleData &Data);
    void SpawnObstacle(const FObstacleData &Data);
    void OnLoginResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    bool ParseGeomWKT(const FString &GeomString, double &OutLon, double &OutLat, double &OutElev);

    FTimerHandle TimerHandle;
    FTimerHandle TokenRefreshHandle;
    TArray<AActor *> SpawnedObstacles;
};