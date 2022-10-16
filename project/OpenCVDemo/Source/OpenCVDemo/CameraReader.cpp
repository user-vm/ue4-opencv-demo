#include "CameraReader.h"

#include "HighResScreenshot.h"
#include "ImageUtils.h"
#include "Transform.h"
#include "Animation/AnimCurveCompressionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

ACameraReader::ACameraReader(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	rootComp = CreateDefaultSubobject<USceneComponent>("Root");
	//Screen_Raw = CreateDefaultSubobject<UStaticMeshComponent>("Screen_Raw");
	Screen_Post = CreateDefaultSubobject<UStaticMeshComponent>("Screen_Post");

	Brightness = 0;
	Multiply = 1;

	// Initialize OpenCV and webcam, properties
	CameraID = 0; //try CAP_DSHOW instead of 0 on Windows if 0 doesn't work
	VideoTrackID = 0;
	isStreamOpen = false;

	//can set width and height in the blueprint to a very high value (like 10000) when using cap.read and the
	//OpenCV webcam API, to make the camera use the maximum resolution it supports.
	//The video size will be corrected to this maximum resolution.
	//Using an unsupported resolution value will not cause a failure. cap will just default to some supported resolution, and VideoSize will be updated to this supported value.
	VideoSize = FVector2D(1920, 1080);
	
	RefreshRate = 30.0f;

	UseOpenCVWebcamReading = false;
	arucoDictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
}

bool SaveBitmapAsPNG(int32 sizeX, int32 sizeY, const TArray<FColor>& bitmapData, const FString& filePath) {
	TArray<uint8> compressedBitmap;
	FImageUtils::CompressImageArray(sizeX, sizeY, bitmapData, compressedBitmap);
	return FFileHelper::SaveArrayToFile(compressedBitmap, *filePath);
}

void SaveTexture2DDebug(UTexture2D* PTexture, FString Filename)
{
	PTexture->UpdateResource();
	FTexture2DMipMap* MM = &PTexture->PlatformData->Mips[0];

	TArray<FColor> OutBMP;
	int w = MM->SizeX;
	int h = MM->SizeY;

	OutBMP.InsertZeroed(0, w*h);

	FByteBulkData* RawImageData = &MM->BulkData;

	FColor* FormatedImageData = reinterpret_cast<FColor*>(RawImageData->Lock(LOCK_READ_ONLY)); //static_cast fails when building for Linux (meaning packaging project, not running in editor, running in editor works)

	for (int i = 0; i < (w*h); ++i)
	{
		OutBMP[i] = FormatedImageData[i];
		OutBMP[i].A = 255;
	}

	RawImageData->Unlock();
	FIntPoint DestSize(w, h);

	SaveBitmapAsPNG(w, h, OutBMP, Filename);
}

void ACameraReader::DetectMarkers(cv::Mat image, cv::Mat imageCameraMatrix, cv::Mat imageDistCoeffs, std::vector<int>& ids, std::vector<cv::Vec3d>& rvecs, std::vector<cv::Vec3d>& tvecs, bool drawAxes) //cv::Mat imageCopy, 
{
	std::vector<std::vector<cv::Point2f>> corners;
	cv::aruco::detectMarkers(image, arucoDictionary, corners, ids); //need to check if ids is sorted; I think it might be, but the function doc isn't clear
	// if at least one marker detected
	if (ids.size() > 0) {
		if (drawAxes)
		{
			cv::aruco::drawDetectedMarkers(image, corners, ids);
		}
		//std::vector<cv::Vec3d> rvecs, tvecs;
		cv::aruco::estimatePoseSingleMarkers(corners, 0.11, imageCameraMatrix, imageDistCoeffs, rvecs, tvecs); //was markerLength 0.05 //0.11 matches their size on the left monitor (Philips)
		UE_LOG(LogTemp, Warning, TEXT("rvecs.size() = %d"), rvecs.size());
		UE_LOG(LogTemp, Warning, TEXT("tvecs.size() = %d"), tvecs.size());
		// draw axis for each marker if requested
		if (drawAxes)
		{
			for(int i=0; i<ids.size(); i++)
			{
				cv::aruco::drawAxis(image, imageCameraMatrix, imageDistCoeffs, rvecs[i], tvecs[i], 0.1);
			}
		}
	} else
	{
		tvecs.resize(0);
		rvecs.resize(0);
		ids.resize(0);
	}
}

//https://gist.github.com/shubh-agrawal/76754b9bfb0f4143819dbd146d15d4c8
//get quaternion from matrix. Should allow for either a 4x4 transformation matrix or a 3x3 rotation matrix.
void getQuaternion(cv::Mat R, double Q[])
{
	double trace = R.at<double>(0,0) + R.at<double>(1,1) + R.at<double>(2,2);
 
	if (trace > 0.0) 
	{
		double s = sqrt(trace + 1.0);
		Q[3] = (s * 0.5);
		s = 0.5 / s;
		Q[0] = ((R.at<double>(2,1) - R.at<double>(1,2)) * s);
		Q[1] = ((R.at<double>(0,2) - R.at<double>(2,0)) * s);
		Q[2] = ((R.at<double>(1,0) - R.at<double>(0,1)) * s);
	} 
    
	else 
	{
		int i = R.at<double>(0,0) < R.at<double>(1,1) ? (R.at<double>(1,1) < R.at<double>(2,2) ? 2 : 1) : (R.at<double>(0,0) < R.at<double>(2,2) ? 2 : 0); 
		int j = (i + 1) % 3;  
		int k = (i + 2) % 3;

		double s = sqrt(R.at<double>(i, i) - R.at<double>(j,j) - R.at<double>(k,k) + 1.0);
		Q[i] = s * 0.5;
		s = 0.5 / s;

		Q[3] = (R.at<double>(k,j) - R.at<double>(j,k)) * s;
		Q[j] = (R.at<double>(j,i) + R.at<double>(i,j)) * s;
		Q[k] = (R.at<double>(k,i) + R.at<double>(i,k)) * s;
	}
}

bool ACameraReader::ReadFrame()
{
	FDateTime StartTime = FDateTime::UtcNow();
	float TimeElapsedInMs;
	//UE_LOG(LogTemp, Warning, TEXT("ReadFrame() called"));
	if (!Camera_Texture2D || !Camera_RenderTarget)
		return false;
	
	//UseOpenCVWebcamReading should be set to true
	if (UseOpenCVWebcamReading)
	{
		if (cap.isOpened())
		{
			//the following can be helpful for checking that the camera framerate is correct. The webcam may default to a low-framerate mode.
			/*TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
			UE_LOG(LogTemp, Warning, TEXT("ACameraReader::ReadFrame() TimeElapsedInMs = %f before cap.read"), TimeElapsedInMs);*/
			const int capReadResult = cap.read(cvMat1);
			DetectMarkers(cvMat1, cameraMatrix, distCoeffs, arucoIds, arucoRvecs, arucoTvecs, drawArucoAxes);
			Aruco_DetectedMarkerIds.SetNum(arucoTvecs.size());
			Aruco_DetectedMarkerPoses.SetNum(arucoTvecs.size());
			UE_LOG(LogTemp, Warning, TEXT("Inner Aruco_DetectedMarkerPoses.Num() = %d"), Aruco_DetectedMarkerPoses.Num());
			FTransform ArucoMarkerPose;
			for (int i=0;i<arucoTvecs.size();i++)
			{
				double arucoMarkerQuaternion[4];
				cv::Mat arucoMarkerPoseMatrix;
				cv::Rodrigues(arucoRvecs[i], arucoMarkerPoseMatrix);
				getQuaternion(arucoMarkerPoseMatrix, arucoMarkerQuaternion);
				ArucoMarkerPose = FTransform(FQuat(arucoMarkerQuaternion[0], arucoMarkerQuaternion[1], arucoMarkerQuaternion[2], arucoMarkerQuaternion[3]),
				FVector(arucoTvecs[i][0]*100, arucoTvecs[i][1]*100, arucoTvecs[i][2]*100)); //Unreal uses cm by default, so multiply by 100
				Aruco_DetectedMarkerIds[i] = arucoIds[i];
				Aruco_DetectedMarkerPoses[i] = ArucoMarkerPose;
			}
			//UE_LOG(LogTemp, Warning, TEXT("capReadResult %d"), capReadResult);
			TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
			UE_LOG(LogTemp, Warning, TEXT("ACameraReader::ReadFrame() TimeElapsedInMs = %f cap.read"), TimeElapsedInMs);
			cvMat1.convertTo(cvMat1, -1, Multiply, Brightness);
			cv::cvtColor(cvMat1, cvMat, cv::COLOR_BGR2BGRA);
			//UE_LOG(LogTemp, Warning, TEXT("%s"), *capReadResult);
			//UE_LOG(LogTemp, Warning, TEXT("cap.isOpened() is true"));
			
		} else
		{
			UE_LOG(LogTemp, Warning, TEXT("cap.isOpened() is false"));
		}
	} else
	{
		// Read the pixels from the RenderTarget and store them in an FColor array
		FRenderTarget* RenderTarget = Camera_RenderTarget->GameThread_GetRenderTargetResource();
		RenderTarget->ReadPixels(ColorData);
		//Get the color data
		cvMat = cv::Mat(cvSize, CV_8UC4, ColorData.GetData());//should it be CV_8UC4 or CV_8UC3? //I think that if using the webcam from the Unreal Engine side, the number of channels can be four, ut then the OpenCV stuff and the Camera_Texture2D need to also have 4 chaannels.
		cvMat.convertTo(cvMat, -1, Multiply, Brightness);
	}
	TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
	UE_LOG(LogTemp, Warning, TEXT("ACameraReader::ReadFrame() TimeElapsedInMs = %f mid"), TimeElapsedInMs);
	//highgui.hpp seems to be missing
	if (OutputDebugImages && !cvMat.empty())
	{
		cv::imwrite("/home/v/output.png", cvMat);
	}

	//Lock the texture so we can read/write to it
	void* TextureData = Camera_Texture2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	const int32 TextureDataSize = ColorData.Num() * 4;
	//set the texture data
	FMemory::Memcpy(TextureData, (void*)(cvMat.data), cvMat.dataend-cvMat.datastart);
	//Unlock the texture
	Camera_Texture2D->PlatformData->Mips[0].BulkData.Unlock();
	//Apply Texture changes to GPU memory
	Camera_Texture2D->UpdateResource();

	if (OutputDebugImages)
	{
		SaveTexture2DDebug(Camera_Texture2D, "/home/v/output_cameratexture.png");
	}
	TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
	UE_LOG(LogTemp, Warning, TEXT("ACameraReader::ReadFrame() TimeElapsedInMs = %f end"), TimeElapsedInMs);
	UE_LOG(LogTemp, Warning, TEXT("Aruco_DetectedMarkerPoses.Num() = %d"), Aruco_DetectedMarkerPoses.Num());
	return true;
}

// Called when the game starts or when spawned
void ACameraReader::BeginPlay()
{
	Super::BeginPlay();
	
	if (UseOpenCVWebcamReading)
	{
		//extract camera matrix and distortion coefficients
		Camera_Matrix.SetNum(9);
		cameraMatrix = (cv::Mat1d(3, 3) << Camera_Matrix[0], Camera_Matrix[1], Camera_Matrix[2], Camera_Matrix[3], Camera_Matrix[4], Camera_Matrix[5], Camera_Matrix[6], Camera_Matrix[7], Camera_Matrix[8]);
		if (Camera_distCoeffs.Num() <= 5)
		{
			Camera_distCoeffs.SetNum(5);
			distCoeffs = (cv::Mat1d(5, 1) << Camera_distCoeffs[0], Camera_distCoeffs[1], Camera_distCoeffs[2], Camera_distCoeffs[3], Camera_distCoeffs[4]);
		}
		else
		{
			Camera_distCoeffs.SetNum(8);
			distCoeffs = (cv::Mat1d(8, 1) << Camera_distCoeffs[0], Camera_distCoeffs[1], Camera_distCoeffs[2], Camera_distCoeffs[3], Camera_distCoeffs[4], Camera_distCoeffs[5], Camera_distCoeffs[6], Camera_distCoeffs[7]);
		}

#ifdef PLATFORM_LINUX
		if(cap.open(CameraID, cv::CAP_V4L2))
#endif
		{
			cap.set(cv::CAP_PROP_FRAME_WIDTH, VideoSize.X);
			cap.set(cv::CAP_PROP_FRAME_HEIGHT, VideoSize.Y);
			VideoSize.X = cap.get(cv::CAP_PROP_FRAME_WIDTH);
			VideoSize.Y = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
			UE_LOG(LogTemp, Warning, TEXT("Opened camera with CameraID %d and set frame size to (%d, %d)"), CameraID, int(VideoSize.X), int(VideoSize.Y));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Could not open camera with CameraID %d"), CameraID);
		}
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("UseOpenCVWebcamReading false"));
	}
	isStreamOpen = true;
	// Prepare the color data array
	ColorData.AddDefaulted(VideoSize.X * VideoSize.Y);
	//setup OpenCV
	cvSize = cv::Size(VideoSize.X, VideoSize.Y);
	cvMat = cv::Mat(cvSize, CV_8UC4);
	// create dynamic texture
	Camera_Texture2D = UTexture2D::CreateTransient(VideoSize.X, VideoSize.Y, PF_B8G8R8A8);

#if WITH_EDITORONLY_DATA
	Camera_Texture2D->MipGenSettings = TMGS_NoMipmaps;
#endif
	Camera_Texture2D->SRGB = Camera_RenderTarget->SRGB;
}

// Called every frame
void ACameraReader::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	RefreshTimer += DeltaTime;
	if (isStreamOpen && RefreshTimer >= 1.0f / RefreshRate)
	{
		RefreshTimer -= 1.0f / RefreshRate;
		ReadFrame();
		OnNextVideoFrame();
	}
}

