##Travel Agency

Simple console application that manages a travel agency.

###About

- This project was made in 2013 and it was developed in C using Kate. 
- The project itself is in Portuguese, although all the code and comments are in English.
- This project was not built to be used, but to illustrate client-server communication using named pipes in Unix.
- This project is not const corrected on purpose.

###Features

- Server:
  - Stores and manages all the data.
  - There is no direct interaction, it executes in background.
  - Does not allow multiple instances.

- Admin:
  - Add or remove cities.
  - Add or remove flights.
  - Add or remove travel agents.
  - See past flights.
  - Change server time.
  - Client administration.
  - Does not allow multiple instances.

- Client
  - Search for flights.
  - Get flights information.
  - Schedule or cancel a flight.