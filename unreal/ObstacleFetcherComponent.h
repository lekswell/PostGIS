#pragma once

#include "CesiumGeoreference.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "JsonUtilities/Public/JsonUtilities.h"
#include "Runtime/Online/HTTP/Public/Http.h"
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
    FString Type;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObstaclesReceived, const TArray<FObstacleData> &, Obstacles);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class JET_API UObstacleFetcherComponent : public UActorComponent
{
    GENERATED_BODY()

  public:
    UObstacleFetcherComponent();

    UFUNCTION(BlueprintCallable, Category = "Obstacle Fetcher")
    void FetchObstacles();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Fetcher")
    bool bUseCameraLocation = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Fetcher")
    AActor *TrackedActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Fetcher")
    FString ServerURL;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Obstacle Fetcher")
    ACesiumGeoreference *Georeference;

    UPROPERTY(BlueprintAssignable, Category = "Obstacle Fetcher")
    FOnObstaclesReceived OnObstaclesReceived;

  protected:
    virtual void BeginPlay() override;

  private:
    TArray<FString> SpawnedObstacleGUIDs; // Список GUID для отслеживания отрисованных объектов
    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
    void ClearObstacles(const TArray<FObstacleData> &NewObstacles); // Обновленная версия очистки препятствий
    void SpawnObstacle(const FObstacleData &Data);

    FTimerHandle TimerHandle;
    TArray<AActor *> SpawnedObstacles;
};
