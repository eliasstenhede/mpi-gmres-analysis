#include <mpi.h>
#include <math.h>
#include "operations.hpp"
#include <iostream>
#include "timer.hpp"

void apply_stencil3d(stencil3d const* S, block_params const* BP, double const* u, double* v) {
	Timer timerstencil("3. Stencil operation for MPI_Cart");

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	//TODO will these fit on the stack for our grid sizes? 
	double send_west_buffer[BP->by_sz*BP->bz_sz];
	double recv_west_buffer[BP->by_sz*BP->bz_sz]; 
	double send_east_buffer[BP->by_sz*BP->bz_sz];  
	double recv_east_buffer[BP->by_sz*BP->bz_sz];  
	
	double send_south_buffer[BP->bx_sz*BP->bz_sz];    
	double recv_south_buffer[BP->bx_sz*BP->bz_sz];    
	double send_north_buffer[BP->bx_sz*BP->bz_sz];    
	double recv_north_buffer[BP->bx_sz*BP->bz_sz];    
	
	double send_bot_buffer[BP->bx_sz*BP->by_sz];   
	double recv_bot_buffer[BP->bx_sz*BP->by_sz];  
	double send_top_buffer[BP->bx_sz*BP->by_sz]; 
	double recv_top_buffer[BP->bx_sz*BP->by_sz]; 
	
	//If we have neighbour in a direction, we communicate the bdry points both ways, otherwise we set the recv_buffer to zero.
	MPI_Request requests[12];
	MPI_Status statuses[12];
	MPI_Comm comm = cart_comm;
	int west_rank, east_rank, south_rank, north_rank, bottom_rank, top_rank;
	MPI_Cart_shift(cart_comm, 0, -1, &rank, &west_rank);
	MPI_Cart_shift(cart_comm, 0, 1, &rank, &east_rank);
	MPI_Cart_shift(cart_comm, 1, -1, &rank, &south_rank);
	MPI_Cart_shift(cart_comm, 1, 1, &rank, &north_rank);
	MPI_Cart_shift(cart_comm, 2, -1, &rank, &bottom_rank);
	MPI_Cart_shift(cart_comm, 2, 1, &rank, &top_rank);
	//west
	if (west_rank != MPI_PROC_NULL) {
		int id = 0;
		for (int iz = 0; iz < BP->bz_sz; iz++) {
			for (int iy = 0; iy < BP->by_sz; iy++, id++) {
				send_west_buffer[id] = u[S->index_c(0, iy, iz)];
			}
		}
		MPI_Isend(send_west_buffer, BP->by_sz * BP->bz_sz, MPI_DOUBLE, west_rank, 0, comm, &requests[0]);
		MPI_Irecv(recv_west_buffer, BP->by_sz * BP->bz_sz, MPI_DOUBLE, west_rank, 1, comm, &requests[1]);
	} else {
		 for (int id=0; id<BP->by_sz*BP->bz_sz; id++) {
			 recv_west_buffer[id]=0.0;
		 }
	}
	//east
	if (east_rank != MPI_PROC_NULL) {
		int id = 0;
		for (int iz = 0; iz < BP->bz_sz; iz++) {
			for (int iy = 0; iy < BP->by_sz; iy++, id++) {
				send_east_buffer[id] = u[S->index_c(BP->bx_sz-1, iy, iz)];
			}
		}
		MPI_Isend(send_east_buffer, BP->by_sz * BP->bz_sz, MPI_DOUBLE, east_rank, 1, comm, &requests[2]);
		MPI_Irecv(recv_east_buffer, BP->by_sz * BP->bz_sz, MPI_DOUBLE, east_rank, 0, comm, &requests[3]);
	} else {
		 for (int id=0; id<BP->by_sz*BP->bz_sz; id++) {
			 recv_east_buffer[id]=0.0;
		 }
	}
	//south
	if (south_rank != MPI_PROC_NULL) {
		int id = 0;
		for (int iz = 0; iz < BP->bz_sz; iz++) {
			for (int ix = 0; ix < BP->bx_sz; ix++, id++) {
				send_south_buffer[id] = u[S->index_c(ix, 0, iz)];
			}
		}
		MPI_Isend(send_south_buffer, BP->bx_sz * BP->bz_sz, MPI_DOUBLE, south_rank, 0, comm, &requests[4]);
		MPI_Irecv(recv_south_buffer, BP->bx_sz * BP->bz_sz, MPI_DOUBLE, south_rank, 1, comm, &requests[5]);
	} else {
		for (int id=0; id<BP->bx_sz*BP->bz_sz; id++) {
			 recv_south_buffer[id]=0.0;
		}
	}
	//north
	if (north_rank != MPI_PROC_NULL) {
		int id = 0;
		for (int iz = 0; iz < BP->bz_sz; iz++) {
			for (int ix = 0; ix < BP->bx_sz; ix++, id++) {
				send_north_buffer[id] = u[S->index_c(ix, BP->by_sz-1, iz)];
			}
		}
		MPI_Isend(send_north_buffer, BP->bx_sz * BP->bz_sz, MPI_DOUBLE, north_rank, 1, comm, &requests[6]);
		MPI_Irecv(recv_north_buffer, BP->bx_sz * BP->bz_sz, MPI_DOUBLE, north_rank, 0, comm, &requests[7]);
	} else {
		 for (int id=0; id<BP->bx_sz*BP->bz_sz; id++) {
			 recv_north_buffer[id]=0.0;
		 }
	}
	//bot
	if (bottom_rank != MPI_PROC_NULL) {
		int id = 0;
		for (int iy = 0; iy < BP->by_sz; iy++) {
			for (int ix = 0; ix < BP->bx_sz; ix++, id++) {
				send_bot_buffer[id] = u[S->index_c(ix, iy, 0)];
			}
		}
		MPI_Isend(send_bot_buffer, BP->bx_sz * BP->by_sz, MPI_DOUBLE, bottom_rank, 0, comm, &requests[8]);
		MPI_Irecv(recv_bot_buffer, BP->bx_sz * BP->by_sz, MPI_DOUBLE, bottom_rank, 1, comm, &requests[9]);
	} else {
		 for (int id=0; id<BP->by_sz*BP->bx_sz; id++) {
			 recv_bot_buffer[id]=0.0;
		 }
	}
	//top
	if (top_rank != MPI_PROC_NULL) {
		int id = 0;
		for (int iy = 0; iy < BP->by_sz; iy++) {
			for (int ix = 0; ix < BP->bx_sz; ix++, id++) {
				send_top_buffer[id] = u[S->index_c(ix, iy, BP->bz_sz-1)];
			}
		}
		MPI_Isend(send_top_buffer, BP->bx_sz * BP->by_sz, MPI_DOUBLE, top_rank, 1, comm, &requests[10]);
		MPI_Irecv(recv_top_buffer, BP->bx_sz * BP->by_sz, MPI_DOUBLE, top_rank, 0, comm, &requests[11]);
	} else {
		 for (int id=0; id<BP->by_sz*BP->bx_sz; id++) {
			 recv_top_buffer[id]=0.0;
		 }
	}
	
	//Wait for recv buffers to be filled
	int neighbours[] = {west_rank, east_rank, south_rank, north_rank, bottom_rank, top_rank};
	for (int id=0; id<6; id++)
		if (neighbours[id] != MPI_PROC_NULL)
			MPI_Wait(&requests[2*id+1], MPI_STATUS_IGNORE);
	
	//Use buffers for edge points, apply the stencil.
	for (int iz=0; iz<BP->bz_sz; iz++) {
		for (int iy=0; iy<BP->by_sz; iy++) {
			for (int ix=0; ix<BP->bx_sz; ix++) {
				int ew_id = BP->by_sz*iz+iy; 
				int ns_id = BP->bx_sz*iz+ix; 
				int tb_id = BP->bx_sz*iy+ix; 
				double accum = S->value_c * u[S->index_c(ix, iy, iz)];
				accum += S->value_w * ((ix==0            ) ? recv_west_buffer[ew_id]  : u[S->index_w(ix, iy, iz)]);
				accum += S->value_e * ((ix==0+BP->bx_sz-1) ? recv_east_buffer[ew_id]  : u[S->index_e(ix, iy, iz)]);
				accum += S->value_s * ((iy==0            ) ? recv_south_buffer[ns_id] : u[S->index_s(ix, iy, iz)]);
				accum += S->value_n * ((iy==0+BP->by_sz-1) ? recv_north_buffer[ns_id] : u[S->index_n(ix, iy, iz)]);
				accum += S->value_b * ((iz==0            ) ? recv_bot_buffer[tb_id]   : u[S->index_b(ix, iy, iz)]);
				accum += S->value_t * ((iz==0+BP->bz_sz-1) ? recv_top_buffer[tb_id]   : u[S->index_t(ix, iy, iz)]);
				v[S->index_c(ix, iy, iz)] = accum;
			}
		}
	}
	return;
}
