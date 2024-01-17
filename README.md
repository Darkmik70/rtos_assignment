# rtos_assignment
Assignment for the course of Real Time Operating Systems at UniGe

## Assignment description
Dear students,
I will give you an assignment that helps you pass the exam. 
The assignment is OPTIONAL for all students and will impact 20% on your final mark. Typically, it may increase your score by 2 points if you take a score from 18 to 22; it will increase your score by 1 point if you take 23 to 27. The assignment will not affect a mark in the exam higher than 27.
Please notice that the assignment is OPTIONAL: however, if you choose to submit it, it will be considered to produce your final score.
***
General rules
(1) You can use all the software used during exercises as a starting point.
(2) You need to work on your own. No groups are allowed. If I suspect two students of having cheated, I may ask them to explain the details of their work, and I will consider the score NOT SUFFICIENT if their answers are not satisfactory.
Assgnment

(1) Design an application with 3 threads J1, J2, J3, whose periods are 300ms, 500ms, and 800ms, plus an aperiodic thread J4 in background which is triggered by J2.
(2) The threads shall just "waste time," as we did in the exercise with threads.
(3) Design a simple driver with only open, close, and write system calls.
(4) During its execution, every task 
	(i) opens the special file associated with the driver;
	(ii ) writes to the driver its identifier plus open square brackets (i.e., [1, [2, [3, or [4)
	(iii) close the special files
	(iv) performs operations (i.e., wasting time)
	(v) performs (i)(ii) and (iii) again to write to the driver its identifier, but with closed square brackets (i.e., 1], 2], 3] or 4]).
(5) The write system call writes on the kernel by other threads (if this does not happen, try to increase the computational time of longer tasks).
(6) Finally, modify the code of all tasks to use semaphores. Every thread now protects all its operations (i) to (v) with a semaphore, which prevents other tasks from preempting. Specifically, use semaphores with a priority ceiling access protocol.  
You do not know how to use the priority ceiling protocol with the Linux thread because we have not mentioned this possibility yet. Go to slides 76-81 in the Thread slides and search for additional information on the web! It should not be difficult.
Good work!
Antonio