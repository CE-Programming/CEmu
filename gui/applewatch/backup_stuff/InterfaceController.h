//
//  InterfaceController.h
//  cemu_core WatchKit Extension
//
//  Created by Adrien Bertrand on 2016-01-07.
//  Copyright © 2016 adriweb. All rights reserved.
//

#import <WatchKit/WatchKit.h>
#import <Foundation/Foundation.h>

@interface InterfaceController : WKInterfaceController

@property (retain, nonatomic) IBOutlet WKInterfaceButton* screenButton;

- (IBAction)toggleOrientation:(id)sender;

@end

