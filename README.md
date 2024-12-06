## Commands to test systerm performance:

1. To test network download a large file:
   ```
   curl http://212.183.159.230/1GB.zip > /dev/null
   ```

2. To test memory allocate a large amount:
   ```
   head -c 2500m /dev/zero | tail
   ```
   
3. To test CPU, run some cpu intensive tasks:

   ```
   stress --cpu 1 --timeout 10s
   ```
