
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
	EMPTY  	  = 32
	# Character to draw for crosshair - '+'
	XHAIR	  = 43
	# Length of a Character
	C_LENGTH  = 2

	MIN_X     = 0
	MIN_Y     = 0
	MAX_X     = 79
	MAX_Y     = 24
	HEIGHT	  = 25
	WIDTH     = 80

	C1_LOC    = 18
	C2_LOC    = 38
	C3_LOC    = 58
	CITY_LEN  = 5

	XHAIR_INIT_X = 40
	XHAIR_INIT_Y = 12
	# Game value

	NXT_MSL   = 8 		#next missile block in ebp
	CMD 	  = 8		#command number from stack
	MSL_STRUCT = 33 	#length of missile struct

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

		call update_missiles

		call update_cities

		call update_crosshair

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
		pushl 	%ebp
		movl 	%esp, %ebp
		pushl	%ebx
		pushl 	%esi
		pushl	%edi

		movl 	CMD(%ebp), %ebx 				#load command value from stack
		cmpl 	$0, %ebx
		jl 		invalid_command
		cmpl 	$4, %ebx
		jg 		invalid_command

		jmp 	*jump_table(, %ebx, 4)			


		invalid_command:
		movl 	$-1, %eax 						#if command is invalid, return -1 
		
		popl	%edi
		popl	%esi
		popl	%ebx
		leave
		ret


# ----------------- Functions private to this file -------------------

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

			movl 	MAX_Y, %edx
			imull	%edx, WIDTH, %eax
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

			movl 	MAX_Y, %edx
			imull	%edx, WIDTH, %eax
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

			movl 	MAX_Y, %edx
			imull	%edx, WIDTH, %eax
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
										   #store current location as old location
			movl 	X(%ebx), %edx
			movl 	%edx, old_x_loc
			movl 	Y(%ebx), %edx
			movl 	%edx, old_y_loc

			off_screen:					   #check X, Y for offscreen 
				movl	X(%ebx), %edx
				cmpl	MIN_X,   %dh
				jl   	to_remove
				cmpl 	MAX_X,   %dh
				jg   	to_remove

				movl 	Y(%ebx), %edx
				cmpl 	MIN_Y,   %dh
				jl   	to_remove
				cmpl 	MAX_Y,   %dh
				jg   	to_remove

			should_explode:					#check if at destination, if not, update missile
				movl 	X(%ebx), %edx 		   
				cmpl 	DEST_X   %dh
				jne  	tick_missile

				movl 	Y(%ebx), %edx
				cmpl 	DEST_Y,  %dh
				jne  	tick_missile
											##if at destination, explode
				pushl 	%ebx				   
				call  	missile_explode
				addl  	$4, %esp			#remove callee param from stack
				cmpl	$0, %eax			#if missile_explode returned nonzero value, notify user
				jz		process_explode
				call 	mp1_notify_user

			process_explode:
				decl 	EXPLODED(%ebx)
				cmpl	$0, EXPLODED(%ebx)	#check if exploded = 0
				je 		to_remove
				jmp 	process_frame


			tick_missile:					#tick missile position
				movl 	X(%ebx), %ecx
				movl	VX(%ebx), %edx
				addl  	%edx, X(%ebx)
				movl 	X(%ebx), %edx

				movl 	Y(%ebx), %ecx
				movl	VY(%ebx), %edx
				addl  	%edx, Y(%ebx)
				movl 	Y(%ebx), %edx

				jmp 	process_frame

			to_remove:						#remove missile from screen when above condition met
				pushl 	%ebp
				call 	remove_missle
				addl 	$4, %esp			#remove callee param from stack

											#remove missile from screen
				movl 	old_y_loc, %edx
				imull	%edx, WIDTH, %eax
				addl 	old_x_loc, %eax
				imull 	C_LENGTH, %eax
				movb	BLANK, %cl
				call 	mp1_poke

				jmp 	next_missile

			process_frame: 					#redraw frame
				movl 	old_y_loc, %edx 	#clear old missile
				imull	%edx, WIDTH, %eax
				addl 	old_x_loc, %eax
				imull 	C_LENGTH, %eax
				movb	BLANK, %cl
				call 	mp1_poke				

			 	movl 	Y(%ebx), %edx 		#draw updated missile
			 	imull	%edx, WIDTH, %eax
			 	addl 	X(%ebx), %eax
			 	imull 	C_LENGTH, %eax
			 	movb	C(%ebx), %cl
			 	call 	mp1_poke

			 	movl 	%ebx, PREV 			#current node becomes prev node in next iteration

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

		movl 	NXT_MSL(%ebp), %ebx
		cmpl 	$0, %ebx					#if called on empty, break
		je 		exit_remove

		movl	PREV, %ecx
		cmpl 	%ebx, %ecx 					#if equal, we are removing head
		je 		remove_head

		movl 	NEXT(%ebx), %edx			#else, update linked list
		movl	%edx, NEXT(%ecx)
		jmp 	free_missile

		remove_head:						#update head
			movl	(NEXT)%ebx, %ecx
			movl 	%ecx, mp1_missile_list
		
		free_missile:						#delete this node
			pushl 	%ebx
			call 	mp1_free
			addl 	$4, %esp

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

		movl 	$0x0, mp1_missile_list 		#import score and linked list
		movl 	$0x0, mp1_score

		movl 	base_alive, %eax 			#set all cities to be alive
		movb 	$1, %eax
		movb 	$1, 1(%eax)
		movb 	$1, 2(%eax)

		movl	XHAIR_INIT_X, crosshairs_x  #init crosshair
		movl	XHAIR_INIT_Y, crosshairs_y

		popl	%edi
		popl	%esi
		popl	%ebx
		ret

mp1_ioctl_addmissile:
		pushl 	%ebp
		movl 	%esp, %ebp
		pushl	%ebx
		pushl 	%esi
		pushl	%edi

		movl    $8(%ebp), %edx 		#store arg in edx
		movl 	MSL_STRUCT, %ecx
		pushl	%edx				#assign memory for a missile struct
		pushl 	%ecx
		call 	mp1_malloc	
		popl 	%ecx
		popl 	%edx			
		
		pushl 	%ecx				#copy missile
		pushl 	%edx
		pushl 	%eax
		call 	mp1_copy_from_user 				

		cmpl 	$0, %eax			#eax contains whether valid transfer
		jne 	copy_failed
		
		popl 	%eax			
		popl 	%edx			
		popl 	%ecx 			

		
		#valid
		movl 	mp1_missile_list, %ebx
		movl 	%ebx, NEXT(%eax)
		movl 	%eax,mp1_missile_list
		movl 	$0, %eax			#add success

		jmp exit_add 			

		copy_failed:
			popl 	%eax
			popl 	%edx			
			popl 	%ecx 
			pushl 	%eax				#free the allocated memory
			call 	mp1_free				
			popl 	%eax
			movl	$-1, %eax 				#return -1

		exit_add:
			popl	%edi
			popl	%esi
			popl	%ebx
			leave
			ret

mp1_ioctl_movexhairs:
		ret

mp1_ioctl_getstatus:
		ret

mp1_ioctl_endgame:
		ret

jump_table:
.long 	mp1_ioctl_startgame, mp1_ioctl_addmissile, mp1_ioctl_movexhairs, mp1_ioctl_getstatus, mp1_ioctl_endgame
