#include <stdio.h>
#include <string.h>

#define MAX_USERS 10
#define MAX_RESOURCES 10

// Permission enum
typedef enum {
    READ = 1,
    WRITE = 2,
    EXECUTE = 4
} Permission;

// User and Resource types
typedef struct {
    char name[50];
} User, Resource;

// ACLEntry type
typedef struct {
    char username[50];
    int permissions;
} ACLEntry;

// ACLControlledResource type
typedef struct {
    Resource resource;
    ACLEntry entries[MAX_USERS];
    int count;
} ACLControlledResource;

// Capability type
typedef struct {
    char resourceName[50];
    int permissions;
} Capability;

// CapabilityUser type
typedef struct {
    User user;
    Capability capabilities[MAX_RESOURCES];
    int count;
} CapabilityUser;

// Function to print human-readable permissions
void printPermissions(int perm) {
    printf("[");
    if (perm & READ) printf("Read ");
    if (perm & WRITE) printf("Write ");
    if (perm & EXECUTE) printf("Execute");
    printf("]");
}

// Function to check if user has required permission
int hasPermission(int userPerm, int requiredPerm) {
    return (userPerm & requiredPerm) == requiredPerm;
}

// Function to check access via ACL
void checkACLAccess(ACLControlledResource* aclResource, const char* username, int requiredPerm, const char* resourceName) {
    printf("ACL Check: User %s requests ", username);
    printPermissions(requiredPerm);
    printf(" on %s: ", resourceName);
    
    int found = 0;
    for (int i = 0; i < aclResource->count; i++) {
        if (strcmp(aclResource->entries[i].username, username) == 0) {
            found = 1;
            if (hasPermission(aclResource->entries[i].permissions, requiredPerm)) {
                printf("Access GRANTED\n");
            } else {
                printf("Access DENIED\n");
            }
            break;
        }
    }
    
    if (!found) {
        printf("User %s has NO entry for resource %s: Access DENIED\n", username, resourceName);
    }
}

// Function to check access via CBAC
void checkCapabilityAccess(CapabilityUser* capabilityUser, const char* resourceName, int requiredPerm) {
    printf("Capability Check: User %s requests ", capabilityUser->user.name);
    printPermissions(requiredPerm);
    printf(" on %s: ", resourceName);
    
    int found = 0;
    for (int i = 0; i < capabilityUser->count; i++) {
        if (strcmp(capabilityUser->capabilities[i].resourceName, resourceName) == 0) {
            found = 1;
            if (hasPermission(capabilityUser->capabilities[i].permissions, requiredPerm)) {
                printf("Access GRANTED\n");
            } else {
                printf("Access DENIED\n");
            }
            break;
        }
    }
    
    if (!found) {
        printf("User %s has NO capability for %s: Access DENIED\n", capabilityUser->user.name, resourceName);
    }
}

int main() {
    // Create users
    User alice = {"Alice"};
    User bob = {"Bob"};
    User charlie = {"Charlie"};
    User dave = {"Dave"};       // New user
    User eve = {"Eve"};         // New user
    
    // Create resources
    Resource file1 = {"File1"};
    Resource file2 = {"File2"};
    Resource file3 = {"File3"};
    Resource file4 = {"File4"};  // New resource
    Resource file5 = {"File5"};  // New resource
    
    // Set up ACL for File1 (hardcoded)
    ACLControlledResource aclFile1 = {
        file1,
        {
            {"Alice", READ | WRITE},
            {"Bob", READ}
        },
        2
    };
    
    // Set up ACL for File2 (hardcoded)
    ACLControlledResource aclFile2 = {
        file2,
        {
            {"Alice", READ | EXECUTE},
            {"Charlie", WRITE}
        },
        2
    };
    
    // Set up ACL for new resources (hardcoded)
    ACLControlledResource aclFile4 = {
        file4,
        {
            {"Dave", READ | WRITE | EXECUTE},
            {"Eve", READ}
        },
        2
    };
    
    ACLControlledResource aclFile5 = {
        file5,
        {
            {"Alice", WRITE},
            {"Eve", EXECUTE}
        },
        2
    };
    
    // Set up capabilities for users (hardcoded)
    CapabilityUser capAlice = {
        alice,
        {
            {"File1", READ | WRITE},
            {"File2", READ | EXECUTE},
            {"File5", WRITE}
        },
        3
    };
    
    CapabilityUser capBob = {
        bob,
        {
            {"File1", READ},
            {"File3", WRITE}
        },
        2
    };
    
    CapabilityUser capCharlie = {
        charlie,
        {
            {"File2", WRITE},
            {"File3", READ | EXECUTE}
        },
        2
    };
    
    // Set up capabilities for new users (hardcoded)
    CapabilityUser capDave = {
        dave,
        {
            {"File4", READ | WRITE | EXECUTE},
            {"File3", READ}
        },
        2
    };
    
    CapabilityUser capEve = {
        eve,
        {
            {"File4", READ},
            {"File5", EXECUTE}
        },
        2
    };
    
    // Test cases (original)
    printf("=== Original Test Cases ===\n");
    checkACLAccess(&aclFile1, "Alice", READ, "File1");
    checkACLAccess(&aclFile1, "Bob", WRITE, "File1");
    checkACLAccess(&aclFile1, "Charlie", READ, "File1");
    
    checkCapabilityAccess(&capAlice, "File1", WRITE);
    checkCapabilityAccess(&capBob, "File1", WRITE);
    checkCapabilityAccess(&capCharlie, "File2", WRITE);
    
    // New test cases
    printf("\n=== New Test Cases ===\n");
    // Test 1: Dave tries to execute File4 (should be granted)
    checkACLAccess(&aclFile4, "Dave", EXECUTE, "File4");
    checkCapabilityAccess(&capDave, "File4", EXECUTE);
    
    // Test 2: Eve tries to write to File4 (should be denied)
    checkACLAccess(&aclFile4, "Eve", WRITE, "File4");
    checkCapabilityAccess(&capEve, "File4", WRITE);
    
    // Test 3: Alice tries to execute File5 (should be denied)
    checkACLAccess(&aclFile5, "Alice", EXECUTE, "File5");
    checkCapabilityAccess(&capAlice, "File5", EXECUTE);
    
    // Test 4: Eve tries to execute File5 (should be granted)
    checkACLAccess(&aclFile5, "Eve", EXECUTE, "File5");
    checkCapabilityAccess(&capEve, "File5", EXECUTE);
    
    // Test 5: Charlie tries to read File3 (should be granted)
    checkCapabilityAccess(&capCharlie, "File3", READ);
    
    // Test 6: Bob tries to execute File3 (should be denied)
    checkCapabilityAccess(&capBob, "File3", EXECUTE);
    
    return 0;
}