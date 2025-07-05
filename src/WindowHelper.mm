#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

// Global variables to store window and layer
static NSWindow* g_window = nil;
static CAMetalLayer* g_metalLayer = nil;

extern "C" {

void* createWindow(int width, int height, const char* title) {
    NSRect frame = NSMakeRect(1280, 720, width, height);
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
    [NSApp run];
}

}
