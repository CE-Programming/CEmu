//
//  InterfaceController.m
//  cemu_core WatchKit Extension
//
//  Created by Adrien Bertrand on 2016-01-07.
//  Copyright © 2016 adriweb. All rights reserved.
//

#import "InterfaceController.h"
#import <WatchConnectivity/WatchConnectivity.h>
#import <UIKit/UIGraphics.h>
#import "emu.h"
#import "lcd.h"

@interface InterfaceController() <WCSessionDelegate>

@property (strong, nonatomic) WCSession *session;

@end

@implementation InterfaceController

uint16_t framebuffer[320 * 240];
uint32_t outPixel8888[320 * 240];
uint32_t bitfields[] = { 0x01F, 0x000, 0x000 };
CGSize scaleSize;
bool portraitOrientation = true;
bool lcdOff = true;

extern bool doGuiStuff;

-(instancetype)init {
    self = [super init];
    
    if (self) {
        if ([WCSession isSupported]) {
            self.session = [WCSession defaultSession];
            self.session.delegate = self;
            [self.session activateSession];
        }
    }
    
    return self;
}

- (void)awakeWithContext:(id)context {
    [super awakeWithContext:context];
    // Configure interface objects here.
    
    [self.screenButton setBackgroundImage:[UIImage imageNamed:@"icon"]];
    
    CGFloat viewHeight = self.contentFrame.size.height;
    scaleSize = CGSizeMake(viewHeight * 240.0 / 320.0, viewHeight);
        
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        NSString *romPath = [[[NSBundle mainBundle] URLForResource:@"84pce_51" withExtension:@"rom"] path];
        NSLog(@"romPath = %@", romPath);
        rom_image = strdup([romPath UTF8String]);
        
        bool reset_true = true;
        bool success = emu_start();
        
        NSLog(@"started");
        
        if (success) {
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
                while (!exiting) {
                    [self updateLCD];
                    usleep(25000);
                }
            });
            emu_loop(reset_true);
        }
    });
     
    NSLog(@"finished");
}

- (IBAction)toggleOrientation:(id)sender
{
    portraitOrientation = !portraitOrientation;
}


- (UIImage *) convertBitmapRGB16ToUIImage:(const uint16_t*) buffer
                                withWidth:(int) width
                               withHeight:(int) height
{
    size_t outBufferLength = width * height * sizeof(uint32_t);
    
    uint16_t *inPixel565 = (uint16_t*)buffer;
    
    for (unsigned int cnt=0; cnt < width * height; inPixel565++, cnt++)
    {
        outPixel8888[cnt] = ( ((((*inPixel565 >> 11) & 0x1F) * 527) + 23) >> 6 )
                          | ( ((((*inPixel565 >> 5)  & 0x3F) * 259) + 33) >> 6 ) << 8
                          | ( ((( *inPixel565        & 0x1F) * 527) + 23) >> 6 ) << 16;
    }
    
    CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, outPixel8888, outBufferLength, NULL);
    size_t bitsPerComponent = 8;
    size_t bitsPerPixel = 32;
    size_t bytesPerRow = sizeof(uint32_t) * width;
    
    CGColorSpaceRef colorSpaceRef = CGColorSpaceCreateDeviceRGB();
    if(colorSpaceRef == NULL) {
        NSLog(@"Error allocating color space");
        CGDataProviderRelease(provider);
        return nil;
    }
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault | kCGImageAlphaNoneSkipLast;
    CGColorRenderingIntent renderingIntent = kCGRenderingIntentDefault;
    
    CGImageRef iref = CGImageCreate(width,
                                    height,
                                    bitsPerComponent,
                                    bitsPerPixel,
                                    bytesPerRow,
                                    colorSpaceRef,
                                    bitmapInfo,
                                    provider,	// data provider
                                    NULL,		// decode
                                    YES,		// should interpolate
                                    renderingIntent);
    
    uint32_t* pixels = (uint32_t*)malloc(outBufferLength);
    
    if(pixels == NULL) {
        NSLog(@"Error: Memory not allocated for bitmap");
        CGDataProviderRelease(provider);
        CGColorSpaceRelease(colorSpaceRef);
        CGImageRelease(iref);
        return nil;
    }
    
    CGContextRef context = CGBitmapContextCreate(pixels,
                                                 width,
                                                 height,
                                                 bitsPerComponent,
                                                 bytesPerRow,
                                                 colorSpaceRef,
                                                 bitmapInfo);
    
    if(context == NULL) {
        NSLog(@"Error context not created");
        free(pixels);
    }
    
    UIImage *image = nil;
    if(context) {
        
        CGContextDrawImage(context, CGRectMake(0.0f, 0.0f, width, height), iref);
        
        CGImageRef imageRef = CGBitmapContextCreateImage(context);
        
        image = [UIImage imageWithCGImage:imageRef];
        
        CGImageRelease(imageRef);	
        CGContextRelease(context);	
    }
    
    CGColorSpaceRelease(colorSpaceRef);
    CGImageRelease(iref);
    CGDataProviderRelease(provider);
    
    if (pixels) {
        free(pixels);
    }
    
    return image;
}


- (void)updateLCD
{
    if (!doGuiStuff)
    {
        return;
    } else {
        doGuiStuff = false;
    }
    
    if (!(lcd.control & 0x800))
    {
        if (!lcdOff)
        {
            lcdOff = true;
            [self.screenButton setBackgroundImage:nil];
            [self.screenButton setTitle:@"LCD Off"];
        }
        return;
    } else {
        lcdOff = false;
        [self.screenButton setTitle:@""];
    }
        
    lcd_drawframe(framebuffer, bitfields);
    
    UIImage *tmpImage = [self convertBitmapRGB16ToUIImage:(const uint16_t*)framebuffer withWidth:320 withHeight:240];
    
    if (!portraitOrientation)
    {
        UIImage *screenRotated = [[UIImage alloc] initWithCGImage: tmpImage.CGImage
                                                            scale: 1
                                                      orientation: UIImageOrientationRight];
        
        @autoreleasepool {
            UIGraphicsBeginImageContextWithOptions(scaleSize, NO, 0.0);
            [screenRotated drawInRect:CGRectMake(0, 0, scaleSize.width, scaleSize.height)];
            UIImage * resizedImage = UIGraphicsGetImageFromCurrentImageContext();
            UIGraphicsEndImageContext();
            [self.screenButton setBackgroundImage:resizedImage];
        }
        
        [screenRotated release];
    } else {
        [self.screenButton setBackgroundImage:tmpImage];
    }

    [tmpImage release];
}

- (void)willActivate {
    // This method is called when watch view controller is about to be visible to user
    [super willActivate];
}

- (void)didDeactivate {
    // This method is called when watch view controller is no longer visible
    [super didDeactivate];
}

@end



