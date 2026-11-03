### Command used for output
`mpirun -np 1 ./EcoSim -N 20 -M 20 -r 60 -f 25 -g 80 -b 20 -p 4 -s 20 -d 400 -t  5000 -i 100`

### Question Responses

1. What messages need to be sent in order to complete the status update and seek step? Consider which animals will need to query other processes for information, and what information would need to be sent back.
    - Which animal will need to query other process?
        Before the seek step, all ranks preemptively push the animal data to the adjacent zone and update their buffer zone after receiving. Thus, the animals will not query other process during the seek step. Instead, they query the local grid_.animals_cache_.
    - Message required
        1. Message size negotiation:
            - Method: MPI_Alltoall
            - Content: An array of 4 integers. Each integer represents the number of animals that are in the corresponding local buffer zone.
            - Purpose: The sender process tells the adjacent ranks the number of animals it will send, so that the receiver process can allocate the correct amount of memory in its receive buffer.
        2. Payload transfer
            - Method: MPI_Isend, MPI_Irecv, and MPI_Waitall
            - Content: An array raw bytes representing AnimalData
            - Purpose: The sender process transfer the data of local animals locating in the buffer zone of the adjacent ranks to them, so that they can update the animals in their buffer zone.
            - MPI_Isend and MPI_Irecv instead of MPI_Send and MPI_Recv is used to post the request first and to prevent deadlocks.
            - Because the payload messages are non-blocking, MPI_Waitall is used to freeze the simulation until all messages have arrived.

2. Explain how you can structure a query so that it can be sent using MPI with minimal data. How many bytes of data do you have to send per query? Do you need to add anything else to your code to help interpret the message?
    - Payload size: 72 bytes per animal
        - 1 uint64_t variable (id) = 8 bytes
        - 6 int variables (species_id, state, x, y, target_x, target_y) = 6 * 4 = 24 bytes
        - 4 Status variables (3 floats, 1 int) = 4 * 4 = 16 bytes
        - 6 Gene variables (6 floats) = 6 * 4 = 24 bytes
    - Additional need: Because the AnimalData is transfered using MPI_BYTE, message tags is needed to indicate if it is a "buffer zone update" message or a "migration" message
    
3. Explain what messaging is required for the move step.
    1. Message size negotiation:
        - Method: MPI_Alltoall
        - Content: An array of 4 integers. Each integer represents the number of animals migrating to Ranks 0, 1, 2, and 3, respectively.
    2. Payload transfer
        - Methods: MPI_Irecv, MPI_Isend, and MPI_Waitall
        - Content: An array raw bytes representing AnimalData

4. Explain what messaging is required for the interaction step. Consider how best to allow animals that are adjacent to the boundary to drink water on the other side of that boundary.
    - No messaging is required during the Interaction step to drink water across boundaries. Because the map is implemented symmetrically initialised across all ranks (using the same RNG seed), and because drinking water is a read-only action, animals can simply query their local Grid interact with the adjacent water. Moreover, the consumed and regrown grass are also handled and updated in every tick to keep the grid status in sync.

5. Explain what messaging is required to gather the population statistics
    - The population count message
        - Method: MPI_Reduce
        - Content: An array with 2 int (local rabbit and fox count)
        - MPI_SUM is used to sum the local counts of animals while traveling
        - Purpose: Deliver the local population to rank 0 and calculate the sum. Only rank 0 receives message
    - The gene statistics message
        - Content: An array of 24 Floats (the sums and squared sums of 6 genes for both species)
        - Purpose: Deliver the Rank 0 needs the sum and the squared value of all genes statistics to rank 0
    - The grass count message is not required as the grid status is identical across all ranks.