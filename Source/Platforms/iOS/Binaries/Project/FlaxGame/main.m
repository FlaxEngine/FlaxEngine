#import <UIKit/UIKit.h>
#import <Source/Engine/Platform/iOS/iOSApp.h>

int main(int argc, char* argv[])
{
    NSString* appDelegateClassName;
    @autoreleasepool {
        appDelegateClassName = NSStringFromClass([FlaxAppDelegate class]);
    }
    return UIApplicationMain(argc, argv, nil, appDelegateClassName);
}
