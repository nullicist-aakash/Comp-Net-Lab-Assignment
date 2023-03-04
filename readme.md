# Computer Networks Lab Assignment

## Requirements
This project was done as part of computer networks course in BITS Pilani. We were given one day time to implement the following:

1. Client and server code.
2. Client sends:
   - `get <key>`
   - `put <key> <value>`
   - `del <key>`
   - `Bye`
3. Server response:
   - `get` should be successful only if there is a key. Returns appropriate error message if it fails.
   - `put` should be successful only if key doesn't exist. Returns appropriate error message if it fails.
   - `del` should be successful only if key exists. Returns appropriate error message if it fails.
4. Server features:
   - Only accept those messages which are in correct format. Returns appropriate error message in case of invalid command.
   - Supports multiple clients at once and have a database store.
   - `<key>` should be a positive integer value.


## Success?
The project was completed within stipulated time but have one problem: database operations on get are sometimes not working and I have no idea why.
