// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#ifndef PLATFORM_IOS
// When included from generated XCode main.m file
#define PLATFORM_IOS 1
#define FLAXENGINE_API
#endif

#if PLATFORM_IOS

#import <UIKit/UIKit.h>

FLAXENGINE_API
@interface FlaxView : UIView

@end

FLAXENGINE_API
@interface FlaxViewController : UIViewController

@end

FLAXENGINE_API
@interface FlaxAppDelegate : UIResponder <UIApplicationDelegate>

@property(strong, retain, nonatomic) UIWindow* window;
@property(strong, retain, nonatomic) FlaxViewController* viewController;
@property(strong, retain, nonatomic) FlaxView* view;

@end

#endif
