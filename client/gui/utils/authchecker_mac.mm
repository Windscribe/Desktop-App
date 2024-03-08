#import "authchecker_mac.h"
#import <Security/AuthorizationTags.h>

#import <Cocoa/Cocoa.h>
#import <Security/Authorization.h>

AuthChecker_mac *g_AuthChecker_mac = NULL;

static NSString *dummyCommandForAuthCheck = @"/bin/rm";
int maxChars = 128;
int maxCommands = 20;

// Obj-c
@interface AuthCheckerHandler : NSObject
{
    AuthorizationRef authorizationRef;
}

+ (id) sharedInstance;
- (BOOL)isAuthenticated:(NSArray *)forCommands;
- (BOOL)authenticate:(NSArray *)forCommands;
- (void)deauthenticate;
@end

@implementation AuthCheckerHandler : NSObject

+ (id) sharedInstance {
    static id sharedTask = nil;
    if(sharedTask==nil) {
        sharedTask = [[AuthCheckerHandler alloc] init];
    }
    return sharedTask;
}

- (id)init {
    self = [super init];
    authorizationRef = NULL;
    return self;
}

- (void)dealloc {
    NSArray *cmds = [[NSArray alloc] initWithObjects:dummyCommandForAuthCheck, nil];
    if ([self isAuthenticated:cmds])
    {
        [self deauthenticate];
    }
    [cmds release];
    [super dealloc];
}

- (BOOL)authenticate:(NSArray *)forCommands {
    if( ![self isAuthenticated:forCommands] ) {
        [self fetchPassword:forCommands];
    }

    return [self isAuthenticated:forCommands];
}

- (void)deauthenticate {
    if(authorizationRef) {
        AuthorizationFree(authorizationRef, kAuthorizationFlagDestroyRights);
        authorizationRef = NULL;
    }
}

- (BOOL)isAuthenticated:(NSArray *)forCommands {
    AuthorizationRights rights;
    AuthorizationRights *authorizedRights;
    AuthorizationFlags flags;

    NSUInteger numItems = [forCommands count];
    AuthorizationItem *items = (AuthorizationItem *) malloc( sizeof(AuthorizationItem) * numItems );
    char paths[maxCommands][maxChars]; // only handles upto 20 commands with paths upto 128 characters in length

    OSStatus err = 0;
    BOOL authorized = NO;
    int i = 0;

    if(authorizationRef==NULL) {
        rights.count=0;
        rights.items = NULL;

        flags = kAuthorizationFlagDefaults;
        AuthorizationCreate(&rights, kAuthorizationEmptyEnvironment, flags, &authorizationRef);
    }

    if( numItems < 1 )
    {
        free( items);
            return authorized;
        }

        while(  (NSUInteger) i < numItems && i < maxCommands ) {
            [[forCommands objectAtIndex:i] getCString:paths[i] maxLength:maxChars encoding:NSUTF8StringEncoding];

            items[i].name = kAuthorizationRightExecute;
            items[i].value = paths[i];
            items[i].valueLength = [[forCommands objectAtIndex:i] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
            items[i].flags = 0;

            i++;
        }

        rights.count = numItems;
        rights.items = items;

        flags = kAuthorizationFlagExtendRights;

        err = AuthorizationCopyRights(authorizationRef, &rights, kAuthorizationEmptyEnvironment, flags, &authorizedRights);

        authorized = (errAuthorizationSuccess==err);

    if(authorized)
        AuthorizationFreeItemSet(authorizedRights);

    if( items)
    free(items);

    return authorized;
}


- (BOOL)fetchPassword:(NSArray *)forCommands {
    AuthorizationRights rights;
    AuthorizationRights *authorizedRights;
    AuthorizationFlags flags;

    NSUInteger numItems = [forCommands count];
    AuthorizationItem *items = (AuthorizationItem*) malloc( sizeof(AuthorizationItem) * numItems );
    char paths[maxCommands][maxChars];

    OSStatus err = 0;
    BOOL authorized = NO;
    int i = 0;

    if( numItems < 1 )
    {
        free( items);
        return authorized;
    }

    while( (NSUInteger) i < numItems && i < maxCommands )
    {
        // NSLog(@"%@", [forCommands objectAtIndex:i]);
        [[forCommands objectAtIndex:i] getCString:paths[i] maxLength:maxChars encoding:NSUTF8StringEncoding];

        items[i].name = kAuthorizationRightExecute;
        items[i].value = paths[i];
        items[i].valueLength = [[forCommands objectAtIndex:i] lengthOfBytesUsingEncoding:NSUTF8StringEncoding];
        items[i].flags = 0;

        i++;
    }

    rights.count = numItems;
    rights.items = items;

    flags = kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights;

    err = AuthorizationCopyRights(authorizationRef, &rights, kAuthorizationEmptyEnvironment, flags, &authorizedRights);

    authorized = (errAuthorizationSuccess == err);

    if (err != errAuthorizationSuccess) {
        return 1;
    }

    if(authorized) {
        AuthorizationFreeItemSet(authorizedRights);
    }

    free(items);

    return authorized;
}

@end

AuthCheckerHandler *g_macAuthCheckerHandler = nil;

// C++
AuthChecker_mac::AuthChecker_mac(QObject *parent)
{
    g_AuthChecker_mac = this;
    g_macAuthCheckerHandler = [AuthCheckerHandler alloc];
}

AuthChecker_mac::~AuthChecker_mac()
{
    [g_macAuthCheckerHandler dealloc];
}

AuthCheckerError AuthChecker_mac::authenticate()
{
    NSArray *cmds = [[NSArray alloc] initWithObjects:dummyCommandForAuthCheck, nil];
    bool success = [g_macAuthCheckerHandler authenticate:cmds];
    [g_macAuthCheckerHandler deauthenticate]; // no need to maintain authentication
    [cmds release];
    return success ?
                AuthCheckerError::AUTH_NO_ERROR :
                AuthCheckerError::AUTH_AUTHENTICATION_ERROR;
}

