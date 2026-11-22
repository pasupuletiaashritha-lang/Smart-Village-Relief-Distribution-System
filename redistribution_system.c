#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 1. TOPIC: STRUCTURES
typedef struct {
    char name[50];
    char id[20];
    int familySize;
    float monthlyIncome;
    int isServed; // 0 = No, 1 = Yes
} Citizen;

typedef struct {
    int riceStock;     // in kg
    int wheatStock;    // in kg
    int fertilizerPackets;
} Inventory;

// Function Prototypes (Modular Programming)
void registerCitizen();
void checkEligibilityAndDistribute();
void viewInventory();
void updateInventory();
void generateReport();
void initializeInventory();

const char* DATA_FILE = "citizens.txt";
const char* INV_FILE = "inventory.txt";

int main() {
    int choice;
    
    // Ensure inventory file exists at startup
    initializeInventory();

    // 3. TOPIC: CONTROL FLOW & LOOPS
    while(1) {
        printf("\n=============================================\n");
        printf("  SMART VILLAGE RELIEF DISTRIBUTION SYSTEM   \n");
        printf("=============================================\n");
        printf("1. Register New Citizen/Family\n");
        printf("2. Check Eligibility & Distribute Relief\n");
        printf("3. View Current Resource Inventory\n");
        printf("4. Update Inventory Stock (Admin)\n");
        printf("5. Generate Community Audit Report\n");
        printf("6. Exit\n");
        printf("Enter your choice (1-6): ");
        scanf("%d", &choice);
        getchar(); // Clear newline buffer

        switch(choice) {
            case 1: registerCitizen(); break;
            case 2: checkEligibilityAndDistribute(); break;
            case 3: viewInventory(); break;
            case 4: updateInventory(); break;
            case 5: generateReport(); break;
            case 6: 
                printf("\nExiting system. Thank you for serving the community!\n");
                exit(0);
            default: 
                printf("\n[Error] Invalid choice! Please try again.\n");
        }
    }
    return 0;
}

// 5. TOPIC: FUNCTIONS
void initializeInventory() {
    FILE *file = fopen(INV_FILE, "r");
    if (file == NULL) {
        // If file doesn't exist, create it with default stock
        file = fopen(INV_FILE, "w");
        Inventory defaultInv = {500, 300, 100}; 
        fwrite(&defaultInv, sizeof(Inventory), 1, file);
    }
    fclose(file);
}

void registerCitizen() {
    Citizen c;
    // 2. TOPIC: FILE HANDLING
    FILE *file = fopen(DATA_FILE, "a"); 
    
    if (file == NULL) {
        printf("[Error] Could not open data store.\n");
        return;
    }

    printf("\n--- Citizen Registration ---\n");
    printf("Enter Head of Family Name: ");
    fgets(c.name, sizeof(c.name), stdin);
    c.name[strcspn(c.name, "\n")] = 0; // Remove trailing newline

    // 4. TOPIC: ARRAYS & STRINGS
    printf("Enter Unique Ration Card / Aadhaar ID: ");
    scanf("%s", c.id);
    
    printf("Enter Family Size: ");
    scanf("%d", &c.familySize);
    
    printf("Enter Total Monthly Income (INR): ");
    scanf("%float", &c.monthlyIncome);
    
    c.isServed = 0; // New registrations are marked unserved

    // Write structure block directly to file
    fwrite(&c, sizeof(Citizen), 1, file);
    fclose(file);

    printf("\n[Success] Citizen registered successfully!\n");
}

void checkEligibilityAndDistribute() {
    char searchId[20];
    Citizen c;
    Inventory inv;
    FILE *cFile = fopen(DATA_FILE, "r+");
    FILE *iFile = fopen(INV_FILE, "r+");
    int found = 0;

    if (cFile == NULL || iFile == NULL) {
        printf("\n[Error] Data records are empty or missing.\n");
        return;
    }

    printf("\nEnter Citizen ID to check relief eligibility: ");
    scanf("%s", searchId);

    // Read inventory
    fread(&inv, sizeof(Inventory), 1, iFile);

    // Search through citizen file records
    while (fread(&c, sizeof(Citizen), 1, cFile)) {
        if (strcmp(c.id, searchId) == 0) {
            found = 1;
            
            printf("\n--- Family Profile Found ---\n");
            printf("Name: %s\nIncome: %.2f INR\nFamily Size: %d\n", c.name, c.monthlyIncome, c.familySize);

            if (c.isServed == 1) {
                printf("\n[Denied] This family has already received their relief quota for this month.\n");
                break;
            }

            // Socioeconomic Eligibility Check logic
            if (c.monthlyIncome <= 15000) { 
                printf("\n[Approved] Profile matches criteria for Low-Income Relief.\n");
                
                int requiredRice = c.familySize * 5; // 5kg per person
                int requiredWheat = c.familySize * 3; // 3kg per person

                if (inv.riceStock >= requiredRice && inv.wheatStock >= requiredWheat) {
                    // Deduct from stock
                    inv.riceStock -= requiredRice;
                    inv.wheatStock -= requiredWheat;
                    c.isServed = 1;

                    // Update Citizen Record in File
                    fseek(cFile, -sizeof(Citizen), SEEK_CUR);
                    fwrite(&c, sizeof(Citizen), 1, cFile);

                    // Update Inventory Record in File
                    fseek(iFile, 0, SEEK_SET);
                    fwrite(&inv, sizeof(Inventory), 1, iFile);

                    printf("\n[Success] Resources Disbursed:\n");
                    printf("- Rice: %d kg\n- Wheat: %d kg\n", requiredRice, requiredWheat);
                } else {
                    printf("\n[Out of Stock] Insufficient inventory to satisfy full family quota.\n");
                }
            } else {
                printf("\n[Denied] Family income exceeds the BPL (Below Poverty Line) threshold.\n");
            }
            break;
        }
    }

    if (!found) {
        printf("\n[Not Found] No record exists for ID: %s\n", searchId);
    }

    fclose(cFile);
    fclose(iFile);
}

void viewInventory() {
    Inventory inv;
    FILE *file = fopen(INV_FILE, "r");
    if (file == NULL) return;

    fread(&inv, sizeof(Inventory), 1, file);
    fclose(file);

    printf("\n--- Current Community Stock ---\n");
    printf("1. Rice Available   : %d kg\n", inv.riceStock);
    printf("2. Wheat Available  : %d kg\n", inv.wheatStock);
    printf("3. Fertilizers Left : %d packets\n", inv.fertilizerPackets);
}

void updateInventory() {
    Inventory inv;
    FILE *file = fopen(INV_FILE, "r+");
    if (file == NULL) return;

    fread(&inv, sizeof(Inventory), 1, file);

    printf("\n--- Restock Panel (Admin) ---\n");
    printf("Add Rice Stock (kg): ");
    int addRice; scanf("%d", &addRice); inv.riceStock += addRice;

    printf("Add Wheat Stock (kg): ");
    int addWheat; scanf("%d", &addWheat); inv.wheatStock += addWheat;

    printf("Add Fertilizer Packets: ");
    int addFert; scanf("%d", &addFert); inv.fertilizerPackets += addFert;

    fseek(file, 0, SEEK_SET);
    fwrite(&inv, sizeof(Inventory), 1, file);
    fclose(file);

    printf("\n[Success] Inventory updated.\n");
}

void generateReport() {
    Citizen c;
    FILE *file = fopen(DATA_FILE, "r");
    int totalFamilies = 0, servedFamilies = 0;

    if (file == NULL) {
        printf("\n[Info] No active citizen directories found.\n");
        return;
    }

    printf("\n=============================================\n");
    printf("          COMMUNITY AUDIT REPORT            \n");
    printf("=============================================\n");
    printf("%-20s %-15s %-10s\n", "Family Head", "ID Number", "Status");
    printf("---------------------------------------------\n");

    while (fread(&c, sizeof(Citizen), 1, file)) {
        totalFamilies++;
        if (c.isServed) servedFamilies++;
        printf("%-20s %-15s %-10s\n", c.name, c.id, c.isServed ? "SERVED" : "PENDING");
    }
    fclose(file);

    printf("---------------------------------------------\n");
    printf("Total Registered Families: %d\n", totalFamilies);
    printf("Total Families Assisted  : %d\n", servedFamilies);
    printf("Pending/Ineligible       : %d\n", totalFamilies - servedFamilies);
    printf("=============================================\n");
}
