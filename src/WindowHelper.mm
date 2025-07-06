#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/QuartzCore.h>
#include "Renderer.h"


// store these in file‐scope:
CVDisplayLinkRef    gDisplayLink = nullptr;
Renderer           *gRenderer    = nullptr;
CA::MetalLayer     *gLayer       = nullptr;


static CVReturn DisplayLinkCallback( CVDisplayLinkRef dl,
                                     const CVTimeStamp* now,
                                     const CVTimeStamp* outputTime,
                                     CVOptionFlags flagsIn,
                                     CVOptionFlags* flagsOut,
                                     void* userInfo )
{
    @autoreleasepool {
        // draw on the main queue—dispatch back if needed
        dispatch_async(dispatch_get_main_queue(), ^{
            gRenderer->draw(gLayer);
        });
    }
    return kCVReturnSuccess;
}

// Global variables to store window and layer
static NSWindow* g_window = nil;
static CAMetalLayer* g_metalLayer = nil;

extern "C" {

void* createWindow(int width, int height, const char* title) {
    NSRect frame = NSMakeRect(0, 0, width, height);

    // Centre the window on the screen
    NSScreen* mainScreen = [NSScreen mainScreen];
    if (mainScreen) {
        NSRect screenFrame = [mainScreen frame];
        frame.origin.x = (screenFrame.size.width - width) / 2;
        frame.origin.y = (screenFrame.size.height - height) / 2;
    }

    NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
    
    g_window = [[NSWindow alloc] initWithContentRect:frame
                                           styleMask:style
                                             backing:NSBackingStoreBuffered
                                               defer:NO];
    
    [g_window setTitle:[NSString stringWithUTF8String:title]];
    
    // Create content view
    NSView* contentView = [[NSView alloc] init];
    [g_window setContentView:contentView];
    
    // Create metal layer
    g_metalLayer = [CAMetalLayer layer];
    g_metalLayer.device = MTLCreateSystemDefaultDevice();
    g_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    g_metalLayer.frame = contentView.bounds;
    
    // Add layer to view
    [contentView setWantsLayer:YES];
    [contentView setLayer:g_metalLayer];
    
    return (__bridge void*)g_window;
}

void* getMetalLayer(void* window) {
    return (__bridge void*)g_metalLayer;
}

void showWindow(void* window) {
    NSWindow* nsWindow = (__bridge NSWindow*)window;
    [nsWindow makeKeyAndOrderFront:nil];
}

void runApp() {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [NSApp activateIgnoringOtherApps:YES];

    CVDisplayLinkCreateWithActiveCGDisplays(&gDisplayLink);
    CVDisplayLinkSetOutputCallback(gDisplayLink,
                                   &DisplayLinkCallback,
                                   nullptr);
    CVDisplayLinkStart(gDisplayLink);

    [NSApp run];
}

}
