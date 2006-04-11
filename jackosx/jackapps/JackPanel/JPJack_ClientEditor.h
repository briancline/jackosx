#import "Jack.h"

@interface JPJack (ClientEditor)

- (void)openEditorForClient:(JPClient *)c;
- (IBAction)closeClientEditor:(id)sender;

- (IBAction)clientAddInputMultiport:(id)sender;
- (IBAction)clientRemoveInputMultiport:(id)sender;
- (IBAction)clientAddOutputMultiport:(id)sender;
- (IBAction)clientRemoveOutputMultiport:(id)sender;
- (void)updateAddRemoveButtons;

@end
