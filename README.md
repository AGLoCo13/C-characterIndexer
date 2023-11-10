# C-characterIndexer
This C programming project demonstrates the efficient analysis of character frequency in a text file using a multi-process, multi-threaded approach. The program generates a file filled with random characters and then analyzes the frequency of each character through multiple threads working on different segments of the file.

Key Features:

Multi-Process Operations: Utilizes separate processes for writing random characters to a file and reading those characters for analysis.
Threaded Analysis: Employs four threads to process distinct sections of the file, computing the frequency of characters using mutex synchronization for accuracy.
Signal Handling: Incorporates signal handlers for graceful termination, ensuring controlled program exit on receiving SIGINT (Ctrl+C) and SIGTERM signals.
Inter-Process Communication: Utilizes POSIX semaphores for synchronization and resource control between processes.
This project aims to illustrate effective inter-process communication and synchronization methods for concurrent operations, presenting the frequency of each character in the file via a consolidated character index.

Explore the code to understand how multiple processes and threads collaborate to analyze character frequency within a text file efficiently.
