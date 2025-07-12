#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/QuartzCore.h>
#include "Renderer.h"
#include "imgui.h"


// store these in file‐scope:
CVDisplayLinkRef    gDisplayLink = nullptr;
Renderer           *gRenderer    = nullptr;
CA::MetalLayer     *gLayer       = nullptr;
MovementHandler    *gMovement = nullptr;

// Track cursor state and ImGui window visibility
static bool gImGuiWindowVisible = false;

void toggleImGuiWindow() {
    gImGuiWindowVisible = !gImGuiWindowVisible;
    
    // Update cursor state immediately when toggling
    if (gImGuiWindowVisible) {
        CGDisplayShowCursor(kCGDirectMainDisplay);
        CGAssociateMouseAndMouseCursorPosition(true);
    } else if (!gImGuiWindowVisible) {
        CGDisplayHideCursor(kCGDirectMainDisplay);
        CGAssociateMouseAndMouseCursorPosition(false);
    }
}


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

bool isImGuiWindowVisible() {
    return gImGuiWindowVisible;
}

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

    [g_window setAcceptsMouseMovedEvents:YES];
    
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

    // key down
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown handler:^NSEvent*(NSEvent* ev) {
      unichar key = [[ev charactersIgnoringModifiers] characterAtIndex:0];
      
      // Check for F1 key (NSF1FunctionKey)
      if ([ev keyCode] == 122) { // F1 key code
          toggleImGuiWindow();
          return nil;
      }
      
      switch(key) {
        case 'q': [NSApp terminate:nil]; return nil;
        case 'r': {
          gMovement->resetCamera();
          return nil;
        }
        default:
          // Only pass to movement handler if ImGui window is not visible
          if (!gImGuiWindowVisible) {
              gMovement->keyDown(key);
          }
          return nil;
      }
    }];

    // key up
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp handler:^NSEvent*(NSEvent* ev) {
      unichar key = [[ev charactersIgnoringModifiers] characterAtIndex:0];
      
      // Only pass to movement handler if ImGui window is not visible
      if (!gImGuiWindowVisible) {
          gMovement->keyUp(key);
      }
      return nil;
    }];

    // mouse move
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskMouseMoved handler:^NSEvent*(NSEvent* ev) {
      // Only update ImGui IO and handle mouse if ImGui window is visible
      if (gImGuiWindowVisible) {
          ImGuiIO& io = ImGui::GetIO();
          
          // Update mouse position for ImGui
          NSPoint mouseLocation = [g_window mouseLocationOutsideOfEventStream];
          NSRect contentRect = [[g_window contentView] frame];
          
          // Convert to ImGui coordinates (top-left origin)
          float mouseX = mouseLocation.x;
          float mouseY = contentRect.size.height - mouseLocation.y;
          
          io.MousePos = ImVec2(mouseX, mouseY);
      } else {
          // Pass to movement handler when ImGui window is not visible
          gMovement->mouseMove(ev.deltaX, ev.deltaY);
      }
      return nil;
    }];

    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown handler:^NSEvent*(NSEvent* ev) {
      if (gImGuiWindowVisible) {
          ImGuiIO& io = ImGui::GetIO();
          io.MouseDown[0] = true;
      }
      return nil;
    }];

    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseUp handler:^NSEvent*(NSEvent* ev) {
      if (gImGuiWindowVisible) {
          ImGuiIO& io = ImGui::GetIO();
          io.MouseDown[0] = false;
      }
      return nil;
    }];

    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskRightMouseDown handler:^NSEvent*(NSEvent* ev) {
      if (gImGuiWindowVisible) {
          ImGuiIO& io = ImGui::GetIO();
          io.MouseDown[1] = true;
      }
      return nil;
    }];

    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskRightMouseUp handler:^NSEvent*(NSEvent* ev) {
      if (gImGuiWindowVisible) {
          ImGuiIO& io = ImGui::GetIO();
          io.MouseDown[1] = false;
      }
      return nil;
    }];

    // Add scroll wheel handler
    [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskScrollWheel handler:^NSEvent*(NSEvent* ev) {
      if (gImGuiWindowVisible) {
          ImGuiIO& io = ImGui::GetIO();
          io.MouseWheelH += [ev scrollingDeltaX] * 0.1f;
          io.MouseWheel += [ev scrollingDeltaY] * 0.1f;
      }
      return nil;
    }];

    // Initially hide cursor for camera movement (ImGui window starts hidden)
    CGDisplayHideCursor(kCGDirectMainDisplay);
    CGAssociateMouseAndMouseCursorPosition(false);

    [NSApp run];
}

}
