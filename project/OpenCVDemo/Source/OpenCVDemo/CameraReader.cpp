// Fill out your copyright notice in the Description page of Project Settings.


#include "CameraReader.h"

#include "HighResScreenshot.h"
#include "ImageUtils.h"
#include "Transform.h"
#include "Animation/AnimCurveCompressionSettings.h"
//#include "BehaviorTree/BehaviorTreeTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
//#include "opencv2/core/mat.hpp"

ACameraReader::ACameraReader(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	rootComp = CreateDefaultSubobject<USceneComponent>("Root");
	//Screen_Raw = CreateDefaultSubobject<UStaticMeshComponent>("Screen_Raw");
	Screen_Post = CreateDefaultSubobject<UStaticMeshComponent>("Screen_Post");

	Brightness = 0;
	Multiply = 1;

	// Initialize OpenCV and webca, properties
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

//void ACameraReader::OnNextVideoFrame()
//{
//}

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
	
	//FString ResultPath;
	//FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
	//bool bSaved = HighResScreenshotConfig.SaveImage(Filename, OutBMP, DestSize, &ResultPath); //this doesn't work from 4.20 onward. There's some weird queueing thing in HighResScreenshotConfig that could potentially do something.
	/*
	FTexture2DResource* PR = (FTexture2DResource*)PTexture->Resource;

	if (PR)
	{
		uint32 Stride;
		void* buf = RHILockTexture2D(PR->GetTexture2DRHI(), 0, RLM_ReadOnly, Stride, false);
	}*/

	//UE_LOG(LogTemp, Warning, TEXT("UHTML5UIWidget::SaveTexture2DDebug: %d %d"), w,h);
	//UE_LOG(LogTemp, Warning, TEXT("UHTML5UIWidget::SaveTexture2DDebug: %s %d"),*ResultPath,bSaved==true ? 1 : 0);

}
//not used, python script is easier
void ACameraReader::WriteMarkers(FString outputFolderPath)
{
	cv::Mat markerImage;
	cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250); //250 is the number of markers in the dictionary
	for(int i=0;i<=249;i++)
	{
		cv::aruco::drawMarker(dictionary, i, 200, markerImage, 1); //was 23 in the example at https://docs.opencv.org/4.x/d5/dae/tutorial_aruco_detection.html
		//cv::imwrite(std::string(TCHAR_TO_UTF8(*outputFolderPath))+"/marker_" + std::to_string(1000+i).substr(1) + ".png", markerImage);
	}
}

void ACameraReader::DetectMarkers(cv::Mat image, cv::Mat imageCameraMatrix, cv::Mat imageDistCoeffs, std::vector<int>& ids, std::vector<cv::Vec3d>& rvecs, std::vector<cv::Vec3d>& tvecs, bool drawAxes) //cv::Mat imageCopy, 
{
	//cv::Mat image, imageCopy;
	//inputVideo.retrieve(image);
	//image.copyTo(imageCopy);
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
		// draw axis for each marker
		int oldArucoMarkerArrayIndex = 0;
		int newArucoMarkerArrayIndex = 0;
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

	if (!UseImageTextureAsInput)
	{
		if (UseOpenCVWebcamReading)
		{
			//TODO: do it the OpenCV way if UseOpenCVWebcamReading is false
		
			if (cap.isOpened())
			{
				//cap>>cvMat;
				//cap.set(cv::CAP_PROP_FOURCC, cv::CAP_OPENCV_MJPEG);

				//this part is no longer needed, since we are copying cvMat.data directly to TextureData 
				/*
				FRenderTarget* RenderTarget = Camera_RenderTarget->GameThread_GetRenderTargetResource();
				RenderTarget->ReadPixels(ColorData);
				auto ColorDataPointer = ColorData.GetData();
				*/
				
				//Get the color data
				//cvMat = cv::Mat(cvSize, CV_8UC4, ColorData.GetData());
				TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
				UE_LOG(LogTemp, Warning, TEXT("ACameraReader::ReadFrame() TimeElapsedInMs = %f before cap.read"), TimeElapsedInMs);
				const int capReadResult = cap.read(cvMat1);
				DetectMarkers(cvMat1, cameraMatrix, distCoeffs, arucoIds, arucoRvecs, arucoTvecs, drawArucoAxes);
				Aruco_DetectedMarkerIds.SetNum(arucoTvecs.size());
				Aruco_DetectedMarkerPoses.SetNum(arucoTvecs.size());
				UE_LOG(LogTemp, Warning, TEXT("Inner Aruco_DetectedMarkerPoses.Num() = %d"), Aruco_DetectedMarkerPoses.Num());
				FTransform ArucoMarkerPose;
				for (int i=0;i<arucoTvecs.size();i++)
				{
					//FMatrix ArucoMarkerPoses = FMatrix();
					//FQuat arucoMarkerQuaternion = FQuat();
					double arucoMarkerQuaternion[4];
					cv::Mat arucoMarkerPoseMatrix;
					cv::Rodrigues(arucoRvecs[i], arucoMarkerPoseMatrix);
					getQuaternion(arucoMarkerPoseMatrix, arucoMarkerQuaternion);
					ArucoMarkerPose = FTransform(FQuat(arucoMarkerQuaternion[0], arucoMarkerQuaternion[1], arucoMarkerQuaternion[2], arucoMarkerQuaternion[3]),
					FVector(arucoTvecs[i][0]*100, arucoTvecs[i][1]*100, arucoTvecs[i][2]*100)); //Unreal uses cm by default, so multiply by 100
					Aruco_DetectedMarkerIds[i] = arucoIds[i];
					Aruco_DetectedMarkerPoses[i] = ArucoMarkerPose;
					/*
					//keep the old and new ids in sorted arrays
					int j;
					for (j=oldArucoMarkerArrayIndex;j<Aruco_OldMarkers.Num() && Aruco_OldMarkers[j].id <= arucoIds[i];j++) {} //TODO: if ids isn't sorted, either need to sort it or do binary search or OldMarkerIds instead of this
					//check if we have found ids[i] in the old ids
					if (j < Aruco_OldMarkers.Num() && Aruco_OldMarkers[j].id == arucoIds[i]) //ids[i] is in Aruco_OldMarkerIds, so we have already seen this marker on the previous frame
					{
						oldArucoMarkerArrayIndex = j + 1;
					}
					else //it is a new marker, add it to Aruco_NewMarkerIds
					{
						Aruco_NewMarkers[newArucoMarkerArrayIndex].id = ids[i]; //since we also need the rvecs and tvecs, Aruco_NewMarkerIds should be a struct containing the marker id, rvec and tvec
						newArucoMarkerArrayIndex++;
					}*/
				}
				//Aruco_NewMarkers.Sort([](auto markerA, auto markerB) {return markerA.id > markerB.id;}); //sort by id
				
				//cv::Mat matImg = cv::imdecode(cv::Mat(1, cvMat1.dataend-cvMat1.datastart, CV_8UC1, cvMat1.data), cv::IMREAD_UNCHANGED);

				//if (OutputDebugImages)
				//{
				//	cv::imwrite("/home/v/output_matImg.png", cvMat);

				UE_LOG(LogTemp, Warning, TEXT("capReadResult %d"), capReadResult);
				TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
				UE_LOG(LogTemp, Warning, TEXT("ACameraReader::ReadFrame() TimeElapsedInMs = %f cap.read"), TimeElapsedInMs);
				cvMat1.convertTo(cvMat1, -1, Multiply, Brightness);
				cv::cvtColor(cvMat1, cvMat, cv::COLOR_BGR2BGRA);
				//UE_LOG(LogTemp, Warning, TEXT("%s"), *capReadResult);
				//UE_LOG(LogTemp, Warning, TEXT("cap.isOpened() is true"));

				/*
				if (static_cast<void*>(ColorDataPointer) == static_cast<void*>(cvMat.data))
				{
					//UE_LOG(LogTemp, Warning, TEXT("(void*)ColorDataPointer == (void*)cvMat.data; ColorDataPointer = %p ; cvMat.data = %p ; (void*) ColorDataPointer = %p; (void*) cvMat.data = %p"),
					//	ColorDataPointer, cvMat.data, static_cast<void*>(ColorDataPointer), static_cast<void*>(cvMat.data));
				} else
				{
					//UE_LOG(LogTemp, Warning, TEXT("(void*)ColorDataPointer != (void*)cvMat.data; ColorDataPointer = %p ; cvMat.data = %p ; (void*) ColorDataPointer = %p; (void*) cvMat.data = %p"),
					//	ColorDataPointer, cvMat.data, static_cast<void*>(ColorDataPointer), static_cast<void*>(cvMat.data));
				}*/
			} else
			{
				//UE_LOG(LogTemp, Warning, TEXT("cap.isOpened() is false"));
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
	} else
	{
		//FTexture2DMipMap* MyMipMap = &ImageMaterial->GetTex.PlatformData->Mips[0]; //get largest mipmap. 
		//ImageMaterial->GetUsedTextures();
	}
	TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
	UE_LOG(LogTemp, Warning, TEXT("ACameraReader::ReadFrame() TimeElapsedInMs = %f mid"), TimeElapsedInMs);
	//highgui.hpp seems to be missing
	if (OutputDebugImages && !cvMat.empty())
	{
		//using "try-catch" doesn't work when packaging, it's an Unreal Engine """feature""". I think there might be some workarounds or alternatives.
		//try
		//{
			//cv::namedWindow("Display",cv::WINDOW_AUTOSIZE);
			//cv::imshow("Display", cvMat); //imshow doesn't work here, asks for libgtk2.0-dev and pkg_config to be installed, but installing it does nothing, probably because of the use of conan.
			cv::imwrite("/home/v/output.png", cvMat);
			//cv::waitKey(0);
			//cv::destroyWindow("Display");
		//}
		/*
		catch( cv::Exception& e )
		{
			const FString err_msg = e.what();
			UE_LOG(LogTemp, Warning, TEXT("exception caught: %s"), *err_msg);
		}*/
	}

	//Lock the texture so we can read/write to it
	void* TextureData = Camera_Texture2D->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	const int32 TextureDataSize = ColorData.Num() * 4;
	//set the texture data
	//FMemory::Memcpy(TextureData, ColorData.GetData(), TextureDataSize);
	//UE_LOG(LogTemp, Warning, TEXT("TextureDataSize = %d; cvMat.isContinuous = %d; cvMat.datastart = %p; cvMat.dataend = %p; cvMat.dataend-cvMat.datastart = %d"),
	//	TextureDataSize, cvMat.isContinuous(), cvMat.datastart, cvMat.dataend, cvMat.dataend-cvMat.datastart);
	FMemory::Memcpy(TextureData, (void*)(cvMat.data), cvMat.dataend-cvMat.datastart); //TextureDataSize);
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

//this is now done from blueprints, by parenting the screen to the camera and setting the distance from it based on the FOV
// /**
//  * @brief 
//  * @return True if position setting succeeded. This will fail (and return false) if the camera is not yet set.
//  */
// bool ACameraReader::SetCameraLocation()
// {
// 	const FTransform& PlaneActorTransform = this->GetTransform();
// 	const FTransform ScreenWorldTransform = Screen_Post->GetComponentTransform();
//
// 	//get bounds in local space, then transform them to global bounds
// 	FVector screenMin;
// 	FVector screenMax;
// 	Screen_Post->GetLocalBounds(screenMin, screenMax);
// 	//get screen width and height in world coordinates
// 	FVector screenWidthWorld = ScreenWorldTransform.ToMatrixWithScale()*(screenMax - screenMin);
// 	FTransform NewCameraPlayerControllerTransform; //TODO: calculate the new transform
// 	UKismetMathLibrary::TransformLocation();
// 	
// 	if (IsValid(CameraPlayerController))
// 	{
// 		FHitResult SweepHitResult; //unused because bSweep is false in K2_SetActorTransform parameters
// 		CameraPlayerController->K2_SetActorTransform(NewCameraPlayerControllerTransform, false, SweepHitResult, false);
// 		return true;
// 	} else
// 	{
// 		return false;
// 	}
// }

// Called when the game starts or when spawned
void ACameraReader::BeginPlay()
{
	Super::BeginPlay();
	
	//cap = cv::VideoCapture("v4l2src device=/dev/video0 ! image/jpeg, format=MJPG ! jpegdec ! video/x-raw,format=BGR ! appsink drop=1", cv::CAP_GSTREAMER);
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

#ifdef PLATFORM_ANDROID
		if(cap.open(CameraID)) //doesn't work. I think this should be done either using the Android Camera Plugin (see https://www.youtube.com/watch?v=4Zm_uOjyImI) or the Get Camera Texture blueprint node from the ARBlueprint library, if you're using ARCore
#else
		if(cap.open(CameraID, cv::CAP_V4L2))
#endif
		//if(cap.open("v4l2src device=/dev/video0 ! image/jpeg, format=MJPG ! jpegdec ! video/x-raw,format=BGR ! appsink drop=1", cv::CAP_GSTREAMER))
		//if(cap.open("v4l2src device=/dev/video0 ! image/jpeg, format=MJPG ! jpegdec ! video/x-raw,format=BGR ! appsink drop=1"))//, cv::CAP_GSTREAMER))
		{
			/*
			int mjpg = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
			
			UE_LOG(LogTemp, Warning, TEXT("UseOpenCVWebcamReading true"));
			const int v4l2Changed = cap.set(cv::CAP_V4L2, 1);
			UE_LOG(LogTemp, Warning, TEXT("v4l2Changed %d"), v4l2Changed);
			const int fourccChanged = cap.set(cv::CAP_PROP_FOURCC, mjpg);//0x47504A4D); //MJPG
			UE_LOG(LogTemp, Warning, TEXT("mjpg %d"), mjpg);
			UE_LOG(LogTemp, Warning, TEXT("fourccChanged %d"), fourccChanged);
			UE_LOG(LogTemp, Warning, TEXT("new cv::CAP_PROP_FOURCC %f"), cap.get(cv::CAP_PROP_FOURCC));
			const int fpsChanged = cap.set(cv::CAP_PROP_FPS, 30);
			UE_LOG(LogTemp, Warning, TEXT("fpsChanged %d"), fpsChanged);
			const int cappropautoexposureChanged = cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 0.75);
			UE_LOG(LogTemp, Warning, TEXT("cappropautoexposureChanged %d"), cappropautoexposureChanged);*/
			
			//icvSetPropertyCAM_V4L( CvCaptureCAM_V4L* capture, int property_id, double value );
			/*
			const int v4l2Changed = cap.set(cv::CAP_V4L2, 1);
			UE_LOG(LogTemp, Warning, TEXT("fpsChanged %d"), v4l2Changed);
			const int fourccChanged = cap.set(cv::CAP_PROP_FOURCC, cv::CAP_OPENCV_MJPEG);//0x47504A4D); //MJPG
			UE_LOG(LogTemp, Warning, TEXT("fourccChanged %d"), fourccChanged);
			const int fpsChanged = cap.set(cv::CAP_PROP_FPS, 30);
			UE_LOG(LogTemp, Warning, TEXT("fpsChanged %d"), fpsChanged);
			const int cappropautoexposureChanged = cap.set(cv::CAP_PROP_AUTO_EXPOSURE, 0.75);
			UE_LOG(LogTemp, Warning, TEXT("cappropautoexposureChanged %d"), cappropautoexposureChanged);*/
			//const int cappropexposureChanged = cap.set(cv::CAP_PROP_EXPOSURE , 3);
			//UE_LOG(LogTemp, Warning, TEXT("cappropexposureChanged %d"), cappropexposureChanged);*/

			//gstreamer example, with which it should be possible to specify v4l2 values
			/*
			//https://stackoverflow.com/a/37057262
			std::string cameraPipeline;
			cameraPipeline ="v4l2src device=/dev/video0 extra-controls=\"c,exposure_auto=1,exposure_absolute=500\" ! ";
			cameraPipeline+="video/x-raw, format=BGR, framerate=30/1, width=(int)1280,height=(int)720 ! ";
			cameraPipeline+="appsink";

			VideoCapture cap;
			cap.open(cameraPipeline);*/

			std::string cameraPipeline;
			cameraPipeline ="v4l2src device=/dev/video0 extra-controls=\"c,exposure_auto=1,exposure_absolute=500\" ! ";
			
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
	//cvMat1 = cv::Mat(cvSize, CV_8UC3);
	cvMat = cv::Mat(cvSize, CV_8UC4); //, ColorData.GetData());
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

