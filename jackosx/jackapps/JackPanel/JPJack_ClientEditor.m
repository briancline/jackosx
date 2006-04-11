#import "JPJack_ClientEditor.h"

@implementation JPJack (ClientEditor)

- (void)openEditorForClient:(JPClient *)c
{	
	NSEnumerator *e = [multiports objectEnumerator];
	JPMultiport *m;
	
	[clientIconImage setImage:[c icon]];
	[clientNameText setStringValue:[c name]];
	[clientDescriptionText setStringValue:[NSString stringWithFormat:@"%d input ports, %d output ports", [[c inputPorts] count], [[c outputPorts] count]]];
	
	editingClient = c;
	
	editingClientInputMultiports = [[NSMutableArray alloc] init];
	editingClientOutputMultiports = [[NSMutableArray alloc] init];
	while (m = [e nextObject])
	{
		if ([m client] == editingClient)
		{
			if ([m type] == kPortTypeInput)
				[editingClientInputMultiports addObject:m];
			else if ([m type] == kPortTypeOutput)
				[editingClientOutputMultiports addObject:m];
		}
	}
	[clientInputPortTable reloadData];
	[clientOutputPortTable reloadData];
	[clientInputMultiportTable reloadData];
	[clientOutputMultiportTable reloadData];
	[self updateAddRemoveButtons];
	
	[NSApp beginSheet:clientEditorPanel modalForWindow:window modalDelegate:self didEndSelector:@selector(clientEditorPanelDidClose:returnCode:contextInfo:) contextInfo:nil];
}

- (IBAction)closeClientEditor:(id)sender
{
    [clientEditorPanel makeFirstResponder:clientEditorPanel];
	[NSApp endSheet:clientEditorPanel];
}

- (void)clientEditorPanelDidClose:(NSWindow *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	[clientEditorPanel orderOut:self];
	
	[editingClientInputMultiports release];
	[editingClientOutputMultiports release];
	editingClientInputMultiports = nil;
	editingClientOutputMultiports = nil;
	[clientInputMultiportTable reloadData];
	[clientOutputMultiportTable reloadData];
	editingClient = nil;
}

//
#pragma mark ADD & REMOVE
//

- (IBAction)clientAddInputMultiport:(id)sender
{
	JPMultiport *newPort = [JPMultiport multiportForClient:editingClient portType:kPortTypeInput];
	
	[multiports addObject:newPort];
	[editingClientInputMultiports addObject:newPort];
	[editingClient setNeedsDisplay];
	[clientInputMultiportTable reloadData];
	[clientInputMultiportTable selectRowIndexes:[NSIndexSet indexSetWithIndex:[clientInputMultiportTable numberOfRows] - 1] byExtendingSelection:NO];
}

- (IBAction)clientRemoveInputMultiport:(id)sender
{
	JPMultiport *port = [editingClientInputMultiports objectAtIndex:[clientInputMultiportTable selectedRow]];
	
	[editingClient setNeedsDisplay];
	[multiports removeObjectIdenticalTo:port];
	[editingClientInputMultiports removeObjectIdenticalTo:port];
	[editingClient setNeedsDisplay];
	[clientInputMultiportTable reloadData];
	[clientInputPortTable reloadData];
}

- (IBAction)clientAddOutputMultiport:(id)sender
{
	JPMultiport *newPort = [JPMultiport multiportForClient:editingClient portType:kPortTypeOutput];
	
	[multiports addObject:newPort];
	[editingClientOutputMultiports addObject:newPort];
	[editingClient setNeedsDisplay];
	[clientOutputMultiportTable reloadData];
	[clientOutputMultiportTable selectRowIndexes:[NSIndexSet indexSetWithIndex:[clientOutputMultiportTable numberOfRows] - 1] byExtendingSelection:NO];
}

- (IBAction)clientRemoveOutputMultiport:(id)sender
{
	JPMultiport *port = [editingClientOutputMultiports objectAtIndex:[clientOutputMultiportTable selectedRow]];

	[editingClient setNeedsDisplay];
	[multiports removeObjectIdenticalTo:port];
	[editingClientOutputMultiports removeObjectIdenticalTo:port];
	[editingClient setNeedsDisplay];
	[clientOutputMultiportTable reloadData];
	[clientOutputPortTable reloadData];
}

- (void)updateAddRemoveButtons
{
	BOOL ai, ri, ao, ro;
	unsigned int i;
	
	ai = ao = ri = ro = YES;
	
	if ([editingClientInputMultiports count] > 0)
	{
		i = [clientInputMultiportTable selectedRow];
		if (i != -1)
			if ([[editingClientInputMultiports objectAtIndex:i] hasConnections])
				ri = NO;
	}
	else
		ri = NO;
	
	if ([editingClientOutputMultiports count] > 0)
	{	
		i = [clientOutputMultiportTable selectedRow];
		if (i != -1)
			if ([[editingClientOutputMultiports objectAtIndex:i] hasConnections])
				ro = NO;
	}
	else
		ro = NO;

	if ([[editingClient inputPorts] count] == 0)
		ai = NO;
		
	if ([[editingClient outputPorts] count] == 0)
		ao = NO;

	[clientAddInputMultiportButton setEnabled:ai];
	[clientRemoveInputMultiportButton setEnabled:ri];
	[clientAddOutputMultiportButton setEnabled:ao];
	[clientRemoveOutputMultiportButton setEnabled:ro];
}

//
#pragma mark TABLE DELEGATE
//

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
	int rows = 0;
	
	if (aTableView == clientInputMultiportTable)
		rows = [editingClientInputMultiports count];
	else if (aTableView == clientInputPortTable)
		rows = [[editingClient inputPorts] count];
	else if (aTableView == clientOutputMultiportTable)
		rows = [editingClientOutputMultiports count];
	else if (aTableView == clientOutputPortTable)
		rows = [[editingClient outputPorts] count];
	
	return rows;
}

- (id)tableView:(NSTableView *)aTableView objectValueForTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
	NSString *identifier = [aTableColumn identifier];
	id obj = nil;
	
	if (aTableView == clientInputMultiportTable)
	{
		obj = [[editingClientInputMultiports objectAtIndex:rowIndex] name];
	}
	else if (aTableView == clientInputPortTable)
	{
		if ([identifier isEqualToString:@"name"])
			obj = [editingClient portNameAtIndex:rowIndex type:kPortTypeInput];
		else if ([identifier isEqualToString:@"used"])
		{
			unsigned int selectedMultiportIndex = [clientInputMultiportTable selectedRow];
			if (selectedMultiportIndex != -1)
				obj = [NSNumber numberWithBool:[[editingClientInputMultiports objectAtIndex:selectedMultiportIndex] referencesPortIndex:rowIndex]];
		}
	}
	else if (aTableView == clientOutputMultiportTable)
	{
		obj = [[editingClientOutputMultiports objectAtIndex:rowIndex] name];
	}
	else if (aTableView == clientOutputPortTable)
	{
		if ([identifier isEqualToString:@"name"])
			obj = [editingClient portNameAtIndex:rowIndex type:kPortTypeOutput];
		else if ([identifier isEqualToString:@"used"])
		{
			unsigned int selectedMultiportIndex = [clientOutputMultiportTable selectedRow];
			if (selectedMultiportIndex != -1)
				obj = [NSNumber numberWithBool:[[editingClientOutputMultiports objectAtIndex:selectedMultiportIndex] referencesPortIndex:rowIndex]];
		}
	}

	return obj;
}

- (void)tableView:(NSTableView *)aTableView setObjectValue:(id)anObject forTableColumn:(NSTableColumn *)aTableColumn row:(int)rowIndex
{
	if (aTableView == clientInputMultiportTable)
	{
		[(JPMultiport *)[editingClientInputMultiports objectAtIndex:rowIndex] setName:anObject];
	}
	else if (aTableView == clientInputPortTable)
	{
		unsigned int selectedMultiportIndex = [clientInputMultiportTable selectedRow];
		if (selectedMultiportIndex != -1)
			[[editingClientInputMultiports objectAtIndex:selectedMultiportIndex] setPortIndex:rowIndex state:[anObject boolValue]];
	}
	else if (aTableView == clientOutputMultiportTable)
	{
		[(JPMultiport *)[editingClientOutputMultiports objectAtIndex:rowIndex] setName:anObject];
	}
	else if (aTableView == clientOutputPortTable)
	{
		unsigned int selectedMultiportIndex = [clientOutputMultiportTable selectedRow];
		if (selectedMultiportIndex != -1)
			[[editingClientOutputMultiports objectAtIndex:selectedMultiportIndex] setPortIndex:rowIndex state:[anObject boolValue]];
	}
}

- (void)tableViewSelectionDidChange:(NSNotification *)aNotification
{
	NSTableView *tableView = [aNotification object];
	
	if (tableView == clientInputMultiportTable)
		[clientInputPortTable reloadData];
	else if (tableView == clientOutputMultiportTable)
		[clientOutputPortTable reloadData];
	
	[self updateAddRemoveButtons];
}

@end
