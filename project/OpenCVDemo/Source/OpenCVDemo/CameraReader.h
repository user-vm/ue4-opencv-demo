// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#define uint64 uint64_opencv
#define int64 int64_opencv
#include "opencv2/core/mat.hpp"
//#include "opencv2/core.hpp"
#include "opencv2/videoio.hpp" //remove "-DBUILD_opencv_videoio=OFF" from conanfile.py if this header cannot be found
//#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/aruco.hpp"
#include "opencv2/calib3d.hpp"
#undef uint64
#undef int64
#include "MediaAssets/Public/MediaTexture.h"
#include "Runtime/Engine/Classes/Engine/Texture2D.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "GameFramework/Actor.h"
//#include "Private/Internationalization/ICURegex.h"
#include "CameraReader.generated.h"
/*
UCLASS()
class UDetectedArucoMarker : public UObject
{
	GENERATED_BODY()
public:
	int id;
	FTransform pose;
};*/
/*
UCLASS()
class UArucoIDActorCorrespondence : public UObject
{
	GENERATED_BODY()
public:
	int ID;
	AActor ActorToPlace;
};*/

UCLASS()
class OPENCVDEMO_API ACameraReader : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACameraReader(const FObjectInitializer& ObjectInitializer);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		USceneComponent* rootComp;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		UStaticMeshComponent* Screen_Post;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera, meta=(ClampMin=0, UIMin=0))
		int32 CameraID;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera, meta=(ClampMin=0, UIMin=0))
		int32 VideoTrackID;
	//the rate at which the color data array and video texture is update (in frames per second)
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera, meta=(ClampMin=0, UIMin=0))
		float RefreshRate;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		float RefreshTimer;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		bool UseOpenCVWebcamReading;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		bool OutputDebugImages;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		bool UseImageTextureAsInput; //apply OpenCV changes on a fixed image
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		bool drawArucoAxes; //draw axes on top of detected Aruco markers in the frame
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		APlayerController* CameraPlayerController; //needed to place the camera player (that controls the position of the camera) so that the CameraReader plane fills the screen
	UPROPERTY(BlueprintReadWrite, Category=Camera)
		bool isStreamOpen;
	UPROPERTY(BlueprintReadWrite, Category=Camera)
		FVector2D VideoSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		float Brightness;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		float Multiply;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		UMediaPlayer* Camera_MediaPlayer;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		UMediaTexture* Camera_MediaTexture;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		UTextureRenderTarget2D* Camera_RenderTarget;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		UTextureRenderTarget2D* Image_RenderTarget;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		UMaterial* ImageMaterial;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		UMaterialInstanceDynamic* Camera_MatRaw;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		UMaterialInstanceDynamic* Camera_MatPost;
	//UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
	//	TArray<UArucoIDActorCorrespondence> ArucoIDActorCorrespondences; //might want to make this a TMap or a UHashTable
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		TMap<int,AActor*> ArucoIDActorMap;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category=Camera)
		UTexture2D* Camera_Texture2D;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category=Camera)
		TArray<FTransform> Aruco_DetectedMarkerPoses;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category=Camera)
		TArray<int> Aruco_DetectedMarkerIds;
	/*
	//this was meant to only spawn objects if they did not exist on the previous frame
	//but what makes more sense is to assign some of the ids to UE actors in the blueprint, and spawn them when the marker is first seen (on isValid), turn off their rendering when their marker is no longer detected, and turn it back on when the marker is seen again
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category=Camera)
		TArray<UDetectedArucoMarker> Aruco_NewMarkers;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category=Camera)
		TArray<UDetectedArucoMarker> Aruco_OldMarkers;
	*/

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		TArray<float> Camera_Matrix; //nested TArrays don't work, so we use a 9-element TArray
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Camera)
		TArray<float> Camera_distCoeffs;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category=Camera)
		TArray<FColor> ColorData;
	// Blueprint Event called every time the video frame is updated
	UFUNCTION(BlueprintImplementableEvent, Category = Camera)
		void OnNextVideoFrame();
	UFUNCTION(BlueprintCallable, Category=Data)
		bool ReadFrame();
	UFUNCTION(BlueprintCallable, Category=Data)
		void WriteMarkers(FString outputFolderPath);
	//UFUNCTION(BlueprintCallable, Category=Data)
	//	bool SetCameraLocation();

	//non-blueprint UE variables
	//TArray<UDetectedArucoMarker>& Aruco_TempMarkers; //helper variable to swap Aruco_NewMarkers and Aruco_OldMarkers

	//OpenCV
	cv::Size cvSize;
	cv::Mat cvMat;
	cv::Mat cvMat1;
	cv::VideoCapture cap;
	cv::Ptr<cv::aruco::Dictionary> arucoDictionary;
	cv::Mat cameraMatrix;
	cv::Mat distCoeffs;
	std::vector<int> arucoIds;
	std::vector<cv::Vec3d> arucoRvecs;
	std::vector<cv::Vec3d> arucoTvecs;

	/**
	 * @brief Aruco marker detection
	 * @param image - input image
	 * @param cameraMatrix - camera matrix
	 * @param distCoeffs - camera distortion coefficients
	 */
	void DetectMarkers(cv::Mat image, cv::Mat imageCameraMatrix, cv::Mat imageDistCoeffs, std::vector<int>& ids, std::vector<cv::Vec3d>& rvecs, std::vector<cv::Vec3d>& tvecs, bool drawAxes); //cv::Mat imageCopy,
	
};
