
# mp1.S - missile-command version
# Good luck, and have fun!

.data
	
    # Constants for accessing the fields of a struct missile, 
    # struct missile is defined in rtc.h and mp1.h

	NEXT      = 0
	X	      = 4
	Y	      = 8
	VX	      = 12
	VY	  	  = 16
	DEST_X	  = 20
	DEST_Y	  = 24
	EXPLODED  = 28
	C	      = 32

    # Character to draw for an explosion - '@'
	EXPLOSION = 64
	# Character to draw for blank - ''
	EMPTY:     .word 32
	# Character to draw for crosshair - '+'
	XHAIR:	   .word 43
	# Length of a Character
	C_LENGTH:  .word 2

MIN_X:     .long 0
MIN_Y:     .long 0
MAX_X:     .long 79
MAX_Y:     .long 24
HEIGHT:	   .long 25
WIDTH:     .long 80

C1_LOC:    .long 18
C2_LOC:    .long 38
C3_LOC:    .long 58
CITY_LEN:  .long 5

XHAIR_INIT_X: .long 40
XHAIR_INIT_Y: .long 12
	# Game value

NXT_MSL=    8 		#next missile block in ebp
CMD = 	    8		#command numsber from stack
MSL_STRUCT: .long 33 	#length of missile struct

# Data shared between this file and rtc.c helper functions

# This '.globl' directive makes these symbols visible externally
.globl mp1_missile_list, base_alive, mp1_score

mp1_missile_list:	.long 0x0	# Head of list pointer
base_alive:		    .long 0x0	# Array of 3 bytes, plus a padding byte
mp1_score:		    .long 0x0	# Player's current score


# Data private to this  file

base_pic:     	    .string "/^^^\\"   # Picture of a live base
dead_base_pic: 	    .string "xxxxx"	   # Picture of a dead base
crosshairs_x:		.long 0x0	       # X-position of the crosshairs
crosshairs_y:		.long 0x0	       # Y-position of the crosshairs

old_x_loc:   		.long 0x0
old_y_loc:   		.long 0x0

PREV: 				.long 0x0		   #keep track of the previous node, 0 if currently at head

.text

# void mp1_poke(void);
# You can use this function to write to video memory.
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:
		movl    vmem_base_addr(,1),%edx
		movb    %cl,(%edx,%eax,1)
		ret

#   ----------------- Exported functions ---------------------

# void mp1_rtc_tasklet(unsigned long garbage);
# Performs three tasks:
#	(1) updates the list of missiles (implement this in update_missiles,
#           below, and call it from here).
#	(2) Redraw the bases - they may have been overwritten with missiles
#	(3) Redraw the crosshairs - it may have been overwritten with missiles
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention

.globl mp1_rtc_tasklet
mp1_rtc_tasklet:
		pushl 	%ebp
		movl 	%esp, %ebp
		pushl	%ebx
		pushl 	%esi
		pushl	%edi

		call 	update_missiles

		cmpl 	$0, %eax
		jz   	draw_city
		call 	mp1_notify_user

		draw_city:
			call 	update_cities

		call 	update_crosshair

		popl	%edi
		popl	%esi
		popl	%ebx
		leave
		ret

# int mp1_ioctl(unsigned long arg, unsigned int cmd)
# The dispatch function for the MP1 ioctls - should use the cmd argument
# and a jumptable to execute one of the specific ioctls implemented below.
# Inputs   : unsigned long arg - parameter to the mp1_ioctl_....
#          : unsigned int cmd  - specifies which mp1_ioctl_... function 
#          :                     to execute
# Outputs  : Returns an integer - depends on which ioctl() function is called
# Registers: Standard C calling convention

.globl mp1_ioctl
mp1_ioctl:
#		pushl 	%ebp
#		movl 	%esp, %ebp
#		pushl	%ebx
#		pushl 	%esi
#		pushl	%edi

		movl 	CMD(%esp), %ebx 				#load command value from stack
		cmpl 	$0, %ebx
		jl 		invalid_command
		cmpl 	$4, %ebx
		jg 		invalid_command

		jmp 	*jump_table(, %ebx, 4)			


		invalid_command:
			movl 	$-1, %eax 						#if command is invalid, return -1 
		
#		popl	%edi
#		popl	%esi
#		popl	%ebx
#		leave
			ret


# ----------------- Functions private to this file -------------------

update_crosshair:
		movl 	crosshairs_y, %eax
		imull 	WIDTH, %eax
		addl 	crosshairs_x, %eax
		imull	C_LENGTH, %eax
		movb 	XHAIR, %cl
		call 	mp1_poke

		ret

update_cities:
		pushl 	%ebp
		movl 	%esp, %ebp
		pushl	%ebx
		pushl 	%esi
		pushl	%edi

		#city 1
		xorl 	%esi, %esi				#use esi as loop counter
		movl 	base_alive, %edi  		#store alive info in edi

		draw_c1: 

			movl 	MAX_Y, %eax
			imull	WIDTH, %eax
			addl 	C1_LOC, %eax
			addl 	%esi, %eax
			imull 	C_LENGTH, %eax

			cmpl 	$0, %edi			#check base alive status
			je 		draw_c1_dead

			draw_c1_live:
			leal 	base_pic, %edx
			jmp 	redraw_c1 	

			draw_c1_dead:
			leal 	dead_base_pic, %edx

			redraw_c1:
			movb	(%edx, %esi, 1), %cl
			call 	mp1_poke
			
			inc 	%esi 				#draw all 5 characters
			cmpl 	CITY_LEN, %esi
			jl 		draw_c1

		#city 2
		xorl 	%esi, %esi				#use esi as loop counter
		movl 	base_alive + 1, %edi  	#store alive info in edi

		draw_c2: 

			movl 	MAX_Y, %eax
			imull	WIDTH, %eax
			addl 	C2_LOC, %eax
			addl 	%esi, %eax
			imull 	C_LENGTH, %eax

			cmpl 	$0, %edi			#check base alive status
			je 		draw_c2_dead

			draw_c2_live:
			leal 	base_pic, %edx
			jmp 	redraw_c2 	

			draw_c2_dead:
			leal 	dead_base_pic, %edx

			redraw_c2:
			movb	(%edx, %esi, 1), %cl
			call 	mp1_poke

			inc 	%esi 				#draw all 5 characters
			cmpl 	CITY_LEN, %esi
			jl 		draw_c2
		
		#city 3
		xorl 	%esi, %esi				#use esi as loop counter
		movl 	base_alive + 2, %edi  	#store alive info in edi

		draw_c3: 

			movl 	MAX_Y, %eax
			imull	WIDTH, %eax
			addl 	C3_LOC, %eax
			addl 	%esi, %eax
			imull 	C_LENGTH, %eax

			cmpl 	$0, %edi			#check base alive status
			je 		draw_c3_dead

			draw_c3_live:
			leal 	base_pic, %edx
			jmp 	redraw_c3 	

			draw_c3_dead:
			leal 	dead_base_pic, %edx

			redraw_c3:
			movb	(%edx, %esi, 1), %cl
			call 	mp1_poke

			inc 	%esi 				#draw all 5 characters
			cmpl 	CITY_LEN, %esi
			jl 		draw_c3

		popl	%edi
		popl	%esi
		popl	%ebx
		leave
		ret


update_missiles:
		pushl 	%ebp
		movl 	%esp, %ebp
		pushl	%ebx
		pushl 	%esi
		pushl	%edi

		movl	mp1_missile_list, %ebx     #load head of linked list
		cmpl 	$0, %ebx
		je 		exit 					   #if list empty, exit

		loop_missiles:

			process_old:
				movl 	X(%ebx), %edx 	   #clear old missile
				shrl	$16, %edx
				movl 	Y(%ebx), %eax 	
				shrl	$16, %eax
				imull 	WIDTH, %eax
				addl 	%edx, %eax
				imull 	C_LENGTH, %eax
				movb	EMPTY, %cl
				call 	mp1_poke	

			off_screen:					   #check X, Y for offscreen 
				movl	X(%ebx), %edx
				shrl 	$16, %edx
				cmpl	MIN_X,   %edx
				jl   	to_remove
				cmpl 	MAX_X,   %edx
				jg   	to_remove

				movl 	Y(%ebx), %edx
				cmpl 	MIN_Y,   %edx
				jl   	to_remove
				cmpl 	MAX_Y,   %edx
				jg   	to_remove

			should_explode:					#check if at destination, if not, update missile
				movl 	X(%ebx), %edx
				shrl 	$16, %edx 		   
				cmpl 	DEST_X(%ebx),  %edx
				jne  	tick_missile

				movl 	Y(%ebx), %edx
				shrl 	$16, %edx 
				cmpl 	DEST_Y(%ebx),  %edx
				jne  	tick_missile
											##if at destination, explode
				pushl 	%ebx				   
				call  	missile_explode
				popl 	%ebx
				cmpl	$0, %eax			
				jne		process_explode
				
			process_explode:
				decl 	EXPLODED(%ebx)
				cmpl	$0, EXPLODED(%ebx)	#check if exploded = 0
				je 		done_explode
				movb 	$EXPLOSION, %cl
				jmp 	process_new

			done_explode:
				movb	EMPTY, %cl
				jmp 	to_remove


			tick_missile:					#tick missile position
				movl 	X(%ebx), %ecx
				movl	VX(%ebx), %edx
				addl  	%edx, X(%ebx)
				movl 	X(%ebx), %edx

				movl 	Y(%ebx), %ecx
				movl	VY(%ebx), %edx
				addl  	%edx, Y(%ebx)
				movl 	Y(%ebx), %edx

				jmp 	process_new

			to_remove:						#remove missile from screen when above condition met
				pushl 	%ebp
				call 	remove_missle
				popl 	%ebp

				jmp 	next_missile

			process_new: 					#redraw frame		
			 	movl 	X(%ebx), %edx 	  
				shrl	$16, %edx
				movl 	Y(%ebx), %eax 	
				shrl	$16, %eax
				imull 	WIDTH, %eax
				addl 	%edx, %eax
				imull 	C_LENGTH, %eax
				call 	mp1_poke	

			next_missile:
				movl 	NEXT(%ebx), %ebx
				cmpl 	$0, %ebx
				jne 	update_missiles	


		exit:
			xorl 	%eax, %eax
			popl	%edi
			popl	%esi
			popl	%ebx
			leave
			ret

remove_missle:
		pushl 	%ebp
		movl 	%esp, %ebp
		pushl	%ebx
		pushl 	%esi
		pushl	%edi

		movl 	8(%ebp), %ebx
		cmpl 	$0, %ebx
		je 		exit_remove
		cmpl 	$0, mp1_missile_list		#if called on empty, break
		je 		exit_remove

		movl 	mp1_missile_list, %edx
		cmpl 	%edx, %ebx
		je 		remove_head

		search_front: 						#find the missile we want to remove
			cmpl 	NEXT(%edx), %ebx
			je 		found_front
			cmpl 	$0, NEXT(%edx)
			je 		exit_remove
			movl 	NEXT(%edx), %edx
			jmp 	search_front

		found_front:
			movl 	NEXT(%ebx), %ecx
			movl 	%ecx, NEXT(%edx)
			pushl 	%ebx
			call 	mp1_free
			addl 	$4, %esp
			movl 	%edx, %ebx
			jmp 	exit_remove

		remove_head:						#update head
			movl	NEXT(%ebx), %ecx
			movl 	%ecx, mp1_missile_list
			pushl 	%ebx
			call 	mp1_free
			addl 	$4, %esp
			movl 	mp1_missile_list, %ebx
			jmp 	exit_remove

		exit_remove: 	
			popl	%edi
			popl	%esi
			popl	%ebx
			leave
			ret

mp1_ioctl_startgame:
		pushl 	%ebp
		movl 	%esp, %ebp
		pushl	%ebx
		pushl 	%esi
		pushl	%edi

		movl 	$0x00010101, base_alive 	#set all cities to be alive

		movl 	$0x0, mp1_missile_list 		#import score and linked list
		movl 	$0x0, mp1_score

		movl	XHAIR_INIT_X, %ecx			#init crosshair
		movl 	%ecx, crosshairs_x  
		movl	XHAIR_INIT_Y, %ecx
		movl	%ecx, crosshairs_y

		popl	%edi
		popl	%esi
		popl	%ebx
		leave
		ret

mp1_ioctl_addmissile:
		pushl 	%ebp
		movl 	%esp, %ebp
		pushl	%ebx
		pushl 	%esi
		pushl	%edi

		movl 	MSL_STRUCT, %ecx
		pushl 	%ecx
		call 	mp1_malloc	
		
		pushl 	8(%ebp)				#copy missile
		pushl 	%eax
		call 	mp1_copy_from_user 		
		popl 	%ecx				#ecx contains node pointer if copy success
		addl 	$8, %esp		

		cmpl 	$0, %eax			#eax contains whether valid transfer
		jne 	copy_failed
				
		#valid
		movl 	mp1_missile_list, %ebx
		movl 	%ebx, NEXT(%ecx)
		movl 	%ecx,mp1_missile_list
		movl 	$0, %eax			#add success

		jmp exit_add 			

		copy_failed:
			pushl 	%ecx				#free the allocated memory
			call 	mp1_free				
			addl 	$4, %esp
			movl	$-1, %eax 				#return -1

		exit_add:
			popl	%edi
			popl	%esi
			popl	%ebx
			leave
			ret

mp1_ioctl_movexhairs:
		pushl	%ebp
		movl	%esp, %ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi

		movw 	8(%ebp), %bx 		#move x and y to cx and dx respectively
		movw 	10(%ebp), %dx

		movl 	crosshairs_x, %esi 	#if command changes xhair out of bounds, skip
		addl 	%ebx, %esi
		cmpl 	MIN_X, %esi
		jl 		exit_move
		cmpl 	MAX_X, %esi
		jg 		exit_move
		movl 	crosshairs_y, %esi
		addl 	%edx, %esi
		cmpl 	MIN_Y, %esi
		jl 		exit_move
		cmpl 	MAX_Y, %esi
		jg 		exit_move

		movl 	crosshairs_y, %esi 	#remove old xhair
		movl	WIDTH, %eax
		imull 	%esi, %eax
		addl 	crosshairs_x, %eax
		imull	C_LENGTH, %eax
		movb	EMPTY, %cl
		call 	mp1_poke

		movl 	crosshairs_y, %esi 	#draw new xhair
		imull 	WIDTH, %esi   							
		movl 	%esi, %eax
		addl 	%ebx, %eax
		imull	C_LENGTH, %eax
		movb	XHAIR, %cl
		call 	mp1_poke

		addl 	%ebx, crosshairs_x	#update xhair coordinates
		movl 	%edx, crosshairs_y

		exit_move:
			popl	%edi
			popl	%esi
			popl	%ebx
			leave
			ret

mp1_ioctl_getstatus:
		pushl	%ebp
		movl	%esp, %ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi

		xor 	%edx, %edx
		movl 	8(%ebp), %ebx 	#get user space pointer

		movl 	mp1_score, %ecx

		check_c1:
			cmpb 	$0, base_alive
			jz 		c1_dead
			xorw 	0x8000, %dx
			jmp 	check_c2

		c1_dead:
			xorw 	0x0000, %dx

		check_c2:
			cmpb 	$0, base_alive +1
			jz 		c2_dead
			xorw 	0x4000, %dx
			jmp 	check_c3

		c2_dead:
			xorw 	0x0000, %dx

		check_c3:
			cmpb 	$0, base_alive +2
			jz 		c3_dead
			xorw 	0x2000, %dx
			jmp 	update_status

		c3_dead:
			xorw 	0x0000, %dx

		update_status:
			pushl	$4
			pushl 	%ecx
			pushw 	%dx
			pushl 	%ebx
			call 	mp1_copy_from_user
			cmpl 	$0, %eax
			popl 	%edx
			addl 	$8, %esp
			jz 		end_update
			movl 	$-1, %eax

		end_update:
			popl	%edi
			popl	%esi
			popl	%ebx
			leave
			ret

mp1_ioctl_endgame:
		pushl	%ebp
		movl	%esp, %ebp
		pushl	%ebx
		pushl	%esi
		pushl	%edi

		cmpl 	$0, mp1_missile_list 	#if list is empty, we are done
		jz 		end_success
		movl 	mp1_missile_list, %ebx

		free_mem:
			movl 	NEXT(%ebx), %ecx

			pushl 	%ebx				#remove this node
			call 	mp1_free
			addl 	$4, %esp

			movl 	%ecx, %ebx			#if there's more, remove again
			cmpl	$0, %ebx
			jne 	free_mem

	end_success:

		popl	%edi
		popl	%esi
		popl	%ebx
		leave
		ret

jump_table:
.long 	mp1_ioctl_startgame, mp1_ioctl_addmissile, mp1_ioctl_movexhairs, mp1_ioctl_getstatus, mp1_ioctl_endgame
