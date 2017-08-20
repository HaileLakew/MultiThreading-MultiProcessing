#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/time.h>

#include <sys/types.h>
#include <sys/file.h>

#include <ctype.h>

int extractInt(FILE *fp) // Used to tokenize file
{
        char ch[1];     // Character bieng looked at will be compare to set values

        ch[0] = getc(fp);

        if(ch[0] >= '0' && ch[0] <= '9')
                return atoi(ch);        // Return int if numerical, else...
        else if( ch[0] == '-')
                return -1;  // Flag for negative number
        else if(ch[0] == ',')
                return -2;  // Flag for comma
        else if(ch[0] == '\n')
                return -3;  // Flag for new line
        else if(ch[0] == EOF)
                return -4;  // Flag for end of file

        return -5;          // Flag for random error
}

int* extractDimensions(FILE *fp) // Extract the dimensions of the matrix in file
{
        int output,
            columns = 0,
            rows = 0;
        static int results[2];


        do {
                output = extractInt(fp); // Extract flag from specifc character

                if(output == -1)
                {
                        output = extractInt(fp);
                        if(rows == 0)
                                columns++;      // Increase column count for each integer value
                }
                else if(output == -3)
                        rows++;                 // Increase row count for each new line
                else if(output > 0)
                {
                        if(rows == 0)
                                columns++;
                }

        } while(output != -4);

        results[0] = rows;      // Return results in a 2D array
        results[1] = columns;

        fseek(fp, 0, SEEK_SET); //Reset file pointer

        return results;
}

int* createMatrix(FILE *fp, int totRow, int totCol)
{
        int *newArray = (int *)malloc(totRow * totCol * sizeof(int)); // Dynamically create a 2D array with dimensions

        int output = 0;

        int row, col;
        for(row = 0; row < totRow; row++)
                for(col = 0; col < totCol; ) // Fill array from file
                {
                        output = extractInt(fp);

                        if(output == -1)
                        {
                                output = extractInt(fp);

                                *(newArray + row*totCol + col) = output* -1;
                                //newArray[row][col] = output* -1;
                                col++;
                        }
                        else if(output >= 0)
                        {
                                *(newArray + row*totCol + col) = output;
                                col++;
                        }
                }

        fseek(fp, 0, SEEK_SET); // Reset file pointer

        return newArray;        // Return finished array
}

int* matrixMult(int *arr1, int *arr2, int totRow, int totCol, int totRow2, int totCol2, FILE *fp) // Multiply Matrices
{
        struct timespec start, stop;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start);  // Start timer

        int     *product = (int *)malloc(totRow * totCol2 * sizeof(int)); // Create 2D array for results (Dynamically)

        int     sum = 0;        // Sum for row calculations

        if(totCol == totRow2)   // Check if matrices are compatiable to multiply
        {
                int row, col, spot;
                for (row = 0; row < totRow; row++)
                        for(col = 0; col < totCol2; col++)
                        {
                                for(spot = 0; spot < totRow2; spot++)
                                        sum += (*(arr1+row*totCol + spot)) * (*(arr2+spot*totCol2 + col)); // Multiply and then sum each component of each array


                                *(product + row*totCol2 + col) = sum; // Save result
                                sum = 0; // Reset sum
                        }

                clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&stop);  // End timer

                // Print Time
                fprintf(fp, "Single Process:\n\tTime Taken::%luns\n", stop.tv_nsec - start.tv_nsec);
                printf("Single Process:\nTIME TAKEN::%luns\n", stop.tv_nsec - start.tv_nsec);

                return product; // Return Product

        }
        else    // If not compatable, inform user, and exit
        {
                printf("Matrices are incompatiable!");
                sleep(3);
                exit(0);
                return product;
        }



}

struct matrix_struct // Method of sending multiple arguments into thread
{
        int* arr1; // First Array
        int* arr2; // Second Array

        int* threadProd; // Array Product

        int totalCol;   // Respective values for array multiplication
        int totalCol2;
        int totalRow2;

        int row;
        /*
                Beginning at 0, each new thread will find a single row of the Array Product array
                        and save that value. Afterwards, it will increase the row count for the next
                        array, which will use that in its calculation for the next row.
        */
};

void *startThread(void *args) // Thread function
{
        int sum = 0;    // Similar as above

        struct matrix_struct *arguments = (struct matrix_struct *)args; // Instatiate argument

        int col, spot;
                for(col = 0; col < arguments->totalCol2; col++) // Find results for only ONE row to the "global (not really)" struct's product array
                {
                        // PAY CLOSE ATTENTION to how "row" is used below
                        for(spot = 0; spot < arguments->totalRow2; spot++)
                                sum += (*(arguments->arr1+arguments->row*arguments->totalCol + spot)) * (*(arguments->arr2+spot*arguments->totalCol2 + col));

                        *(arguments->threadProd + arguments->row*arguments->totalCol2 + col) = sum;
                        sum = 0;
                }

        arguments->row++; // Increase to modify row for the next thread that uses this function

        return NULL;
}
int* threadMult(int *newArray, int *newArray2, int totRow, int totCol, int totRow2, int totCol2, FILE *fp) // Call to threaded version of program
{

        struct timespec start, stop;
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&start);  // Start timer

        pthread_t *tID; // An array of threads, one thread for each possible row
        tID = (pthread_t*) malloc(sizeof(pthread_t) * totRow);

        struct matrix_struct arguments; // Instatiate structure

        /* Set default values before threads execute*/
        arguments.arr1 = newArray;
        arguments.arr2 = newArray2;

        arguments.threadProd = (int *)malloc(totRow * totCol2 * sizeof(int));

        arguments.totalCol = totCol;
        arguments.totalCol2 = totCol2;
        arguments.totalRow2 = totRow2;
        arguments.row = 0;

        int spot;
        for(spot = 0; spot < totRow; spot++)
                pthread_create(&tID[spot], NULL, startThread, (void*)&arguments); // Execute threads in succession

        for(spot = 0; spot < totRow; spot++)
                pthread_join(tID[spot], NULL);  // Make main process wait for all threads to finish

        clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&stop); ;
        // Save time
        fprintf(fp, "Threaded Process:\n\tTime Taken::%luns\n", stop.tv_nsec - start.tv_nsec);
        printf("Threaded Process:\nTIME TAKEN::%luns\n", stop.tv_nsec - start.tv_nsec);

        // Return results
        return arguments.threadProd;
}
int main(void)
{
        FILE *fp = fopen("matA.txt", "r");
        FILE *fp2 = fopen("matB.txt", "r");


        if (fp && fp2) // Check if both files exist
        {
                FILE *answers = fopen("answers.txt", "w"); // Means of write answers to file

                int *dimensions = extractDimensions(fp); // Extract Diminensions from each file

                // Matrix 1
                int     totRow = dimensions[0],
                        totCol = dimensions[1];

                // Matrix 2
                dimensions = extractDimensions(fp2);
                int     totRow2 = dimensions[0],
                        totCol2 = dimensions[1];

                int* newArray = createMatrix(fp, totRow, totCol);       // Save Extracted Matrices
                int* newArray2 = createMatrix(fp2, totRow2, totCol2);

                // Main Single Process Multiplication
                int* product = matrixMult(newArray, newArray2, totRow, totCol, totRow2, totCol2, answers);

                // Report Answers
                int spot, spot2;
                for(spot = 0; spot < totRow; spot++)
                {
                        for(spot2 = 0; spot2 < totCol2; spot2++)
                        {
                                if(spot2 == totCol2 - 1)
                                {
                                        fprintf(answers, "%d", *(product + spot*totCol2 + spot2));
                                        printf("%d", *(product + spot*totCol2 + spot2));
                                        continue;
                                }
                                fprintf(answers, "%d, ", *(product + spot*totCol2 + spot2));
                                printf("%d, ", *(product + spot*totCol2 + spot2));
                        }
                        fprintf(answers, "\n");
                        printf("\n");
                }
                fprintf(answers, "\n--------------------------------------------------------------------------------------\n");
                printf("\n--------------------------------------------------------------------------------------\n");

                // Multithreaded Process Multiplication
                int* threadProduct = threadMult(newArray, newArray2, totRow, totCol, totRow2, totCol2, answers);

                // Report Answers
                for(spot = 0; spot < totRow; spot++)
                {
                        for(spot2 = 0; spot2 < totCol2; spot2++)
                        {
                                if(spot2 == totCol2 - 1)
                                {
                                        fprintf(answers, "%d", *(threadProduct + spot*totCol2 + spot2));
                                        printf("%d", *(threadProduct + spot*totCol2 + spot2));
                                        continue;
                                }

                                fprintf(answers, "%d, ", *(threadProduct + spot*totCol2 + spot2));
                                printf("%d, ", *(threadProduct + spot*totCol2 + spot2));
                        }
                        fprintf(answers, "\n");
                        printf("\n");
                }


                fclose(fp);
                fclose(fp2);
        }
        else
        {
                printf("There is an error reading in files.\n");
                printf("Make sure file is in directory\n");
        }
}
