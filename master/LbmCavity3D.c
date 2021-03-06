#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <float.h>
#include "../header/Argument.h"
#include "../header/LbmMaster.h"

int main(int argc, char *argv[])                                       
{                                                                      
    int ***flags;                                                      
    int i, j, k, s, l;
    int myrank, my2drank, size;
    int current = 0, other = 1; 
    int periods[NUM_DIMS] = {0, 0};
    int dims[NUM_DIMS] = {0, 0};
    int coords[2];
    Real df;
    unsigned int X_section ;
    unsigned int X_res     ;
    unsigned int Y_section ;
    unsigned int Y_res     ;
    int	Xst ;
    int	Xed ;
    int	Yst ;
    int	Yed ;
    int x_sec ;
    int y_sec ;
    Real *****nodes;
    int ****walls;
    Real ***temp_right;
    Real ***temp_left;
    Real ***temp_down;
    Real ***temp_up;
    Real ***temp_right_send;
    Real ***temp_left_send;
    Real ***temp_down_send;
    Real ***temp_up_send;
    Real **temp_lu;
    Real **temp_ld;
    Real **temp_ru;
    Real **temp_rd;
    Real **temp_lu_send;
    Real **temp_ld_send;
    Real **temp_ru_send;
    Real **temp_rd_send;
    MPI_Comm mycomm;
    MPI_Status sta[16];
    MPI_Request req[16];
    int n = 0;
    int count;
    int local_rankinfo[7];
    int **rankinfo;
    Real **image;
    Real **local_image;
		
	/*------------------------
         * Parameter Set
         * ----------------------*/

    	setParameter();
	
	/*---------------------------*
	 * MPI Init
	 * --------------------------*/
    

    	MPI_Init(&argc, &argv);

    	MPI_Comm_size(MPI_COMM_WORLD, &size);

    	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	MLOG("CPC -- LBM-simulation Benchmark!\n\n");
	 
	/*----------------------------*
         * MPI Mesh Section
         * ---------------------------*/
    
	MPI_Dims_create(size, NUM_DIMS, dims);

    	MPI_Cart_create(MPI_COMM_WORLD, NUM_DIMS, dims, periods, 0, &mycomm);

    	MPI_Comm_rank(mycomm, &my2drank);

    	MPI_Cart_coords(mycomm, my2drank, 2, coords);

	SetMPI(mycomm, dims, coords);
	


    	X_section = X / dims[0];
    	X_res     = X % dims[0];
    	Y_section = Y / dims[1];
    	Y_res     = Y % dims[1];
    
	Xst = (coords[0] < X_res) ? (coords[0] * (X_section + 1))
								  : (coords[0] *  X_section + X_res);
	Xed = (coords[0] < X_res) ? (Xst       + (X_section + 1))
								  : (Xst       +  X_section);
	Yst = (coords[1] < Y_res) ? (coords[1] * (Y_section + 1))
								  : (coords[1] *  Y_section + Y_res);
	Yed = (coords[1] < Y_res) ? (Yst       + (Y_section + 1))
								  : (Yst       +  Y_section);
	
    	x_sec = Xed - Xst;
    	y_sec = Yed - Yst;
	

	/*-----------------------------*
         *-----------------------------* 
         * ----------------------------*/
	
	MLOG("Size                 :  %d x %d x %d\n", X, Y, Z);

    //hch
    // STEPS = 10;
    //printf("STEPS = %d\n", STEPS);

        MLOG("Steps                :  %d\n", STEPS);
        MLOG("Number of Process    :  %d\n\n", size);
	
        /*-----------------------------*
         * Space Allocate
         * ----------------------------*/

	

	flags = array3DI(x_sec + 2, y_sec + 2, Z);
	nodes = array5DF(2, x_sec + 2, y_sec + 2, Z, 19);
	walls = array4DI(x_sec, y_sec, Z, 19);
    	temp_lu_send    = array2DF(Z, 19);
    	temp_lu         = array2DF(Z, 19);
    	temp_ld         = array2DF(Z, 19);
    	temp_ru         = array2DF(Z, 19);
    	temp_rd         = array2DF(Z, 19);
    	temp_ld_send    = array2DF(Z, 19);
    	temp_ru_send    = array2DF(Z, 19);
    	temp_rd_send    = array2DF(Z, 19);
    	temp_right      = array3DF(y_sec, Z, 19);
    	temp_left       = array3DF(y_sec, Z, 19);
    	temp_down       = array3DF(x_sec, Z, 19);
    	temp_up         = array3DF(x_sec, Z, 19);
    	temp_right_send = array3DF(y_sec, Z, 19);
    	temp_left_send  = array3DF(y_sec, Z, 19);
    	temp_down_send  = array3DF(x_sec, Z, 19);
    	temp_up_send    = array3DF(x_sec, Z, 19);
    	memset(&walls[0][0][0][0], 0, x_sec * y_sec * Z * 19 * sizeof(int));
	
	/*----------------------------------------*
	 * local rank info
	 * ---------------------------------------*/

	local_image = array2DF(y_sec, x_sec * 5);

	if(my2drank == 0) {
		image = array2DF(Y, X * 5);
		rankinfo = array2DI(size, 7);
	} else {
		image = array2DF(1, 1);
		rankinfo = array2DI(1, 1);
	}

	INITINPUT(X, Y, Z, Xst, Xed, Yst, Yed, x_sec, y_sec, myrank, size, argv[1], local_rankinfo, rankinfo, flags);

	MLOG("Init >> init Pointer!\n");

    	init_Pointer(flags, nodes, walls, Xst, Xed, Yst, Yed, Z, x_sec, y_sec, 1, LDC_VELOCITY);

	MLOG("Step >> Main Steps start!\n\n");
	

	/*----------------------------------------------------*
	 * Main Calculation section
     * ---------------------------------------------------*/
	TIME_ST();

    lbm_data_init(X, Y, Z, Xst, Xed, Yst, Yed, x_sec, y_sec, nodes, flags, walls, STEPS,
                  temp_right,temp_left,temp_down,temp_up,
                  temp_right_send,temp_left_send,temp_down_send,temp_up_send,
                  temp_lu,temp_ld,temp_ru,temp_rd,
                  temp_lu_send,temp_ld_send,temp_ru_send,temp_rd_send);
    main_iter(&s);
    
    TIME_ED();


    //hch_validation field

    //writing

    FILE * outfile, *infile;

    char fname[1000];
	sprintf(fname, "lbm_steps-%d_dump_rank-%02d.dat", STEPS, myrank);
    //sprintf(fname, "_hch_lbm_val_fine_rank_%02d_STEPS_%d.dat", myrank, STEPS);

    infile = fopen(fname, "rb");
    //both "other" and "current" will be checked
    MPI_Barrier(MPI_COMM_WORLD);
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);
    if(infile == NULL)
    {
        //try to write it
        outfile = fopen(fname, "wb");
        if(outfile == NULL)
        {
            printf("fuck you! 怎么写不出来？？");
        }
        else
        {
            //写出整个nodes数组
            int d0, d1, d2, d3;
            for(d0 = 0; d0 < 2; d0++)
            {
                for(d1 = 1; d1 < x_sec + 1; d1++)
                {
                    //for(d2 = 0; d2 < y_sec + 2; d2++)
                    d2 = 1;
                    {
                        for(d3 = 0; d3 < Z; d3++)
                        {
                            //fwrite(nodes[d0][d1][d2][d3], sizeof(Real), 19, outfile);
                            fwrite(nodes[d0][d1][d2][d3], sizeof(Real), 1, outfile);//如果把1改成19，那么会写出2GB的东西
                        }
                    }
                }
            }
            fclose(outfile);
        }
        
    }
    else
    {
//        if(myrank == 0)
//        {
//            //rank0会产生exception，暂时不知道为什么
//        }
//        else
        {
            //read and validation
            //Real* my_check = malloc(sizeof(Real) * 2 * (x_sec + 2) * (y_sec + 2) * Z * 19);
            Real* my_check = malloc(sizeof(Real) * 2 * (x_sec + 2) * (1) * Z * 1);

            fread(my_check, sizeof(Real), 2 * (x_sec + 2) * (1) * Z * 1, infile);
            //fread(my_check, sizeof(Real), 2 * (x_sec + 2) * (y_sec + 2) * Z * 19, infile);

            int d0, d1, d2, d3, d4;
            int iter = 0;
            for(d0 = 0; d0 < 2; d0++)
            {
                for(d1 = 1; d1 < x_sec + 1; d1++)
                {
                    //for(d2 = 0; d2 < y_sec + 2; d2++)
                    d2 = 1;
                    {
                        for(d3 = 0; d3 < Z; d3++)
                        {
                            d4 = 0;
                            //for(d4 = 0; d4 < 19; d4++)
                            {
                                if(isnan(nodes[d0][d1][d2][d3][d4]) || fabs(my_check[iter] - nodes[d0][d1][d2][d3][d4]) > 1e-7)
                                {
                                    printf("myrank = %d, 5d = %d %d %d %d %d, wrong! standard = %.15f, this = %.15f, err = %.15f\n", 
                                    myrank, d0, d1, d2, d3, d4, my_check[iter], 
                                    nodes[d0][d1][d2][d3][d4],
                                    my_check[iter] - nodes[d0][d1][d2][d3][d4]);
                                    iter = -1;
                                    break;
                                }
                                iter++;
                            }
                            if(iter == -1)
                                break;
                        }

                        if(iter == -1)
                            break;
                    }
                    if(iter == -1)
                        break;
                }
                if(iter == -1)
                    break;
            }
            if(iter > 0)
            {
                printf("myrank = %d, STEPS = %d, pass!\n", myrank, STEPS);
            }

            //读入my_check，检查my_check（标准的nodes）和nodes是否相等
        }
        fclose(infile);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    fflush(stdout);
    MPI_Barrier(MPI_COMM_WORLD);


	/*-----------------------------*
 	 * OUTPUT 
 	 *-----------------------------*/

	MLOG("Step >> Main Steps Done!\n\n");

    if(STEPS == 100)
	   OUTPUT(X, Y, Z, Xst, Xed, Yst, Yed, s, myrank, size, other, x_sec, y_sec, argv[1], local_image, image, rankinfo, nodes);

	arrayFree2DF(image);
	arrayFree2DI(rankinfo);
	arrayFree2DF(local_image);

	arrayFree3DI(flags);
	arrayFree5DF(nodes);
	arrayFree4DI(walls);
	arrayFree3DF(temp_right);
	arrayFree3DF(temp_left);
	arrayFree3DF(temp_down);
	arrayFree3DF(temp_up);
	arrayFree2DF(temp_lu);
	arrayFree2DF(temp_ld);
	arrayFree2DF(temp_ru);
	arrayFree2DF(temp_rd);
	arrayFree3DF(temp_right_send);
	arrayFree3DF(temp_left_send);
	arrayFree3DF(temp_down_send);
	arrayFree3DF(temp_up_send);
	arrayFree2DF(temp_lu_send);
	arrayFree2DF(temp_ld_send);
	arrayFree2DF(temp_ru_send);
	arrayFree2DF(temp_rd_send);

	MLOG("LBM-simulation Done!\n");

    	MPI_Finalize();

	return 0;
}
