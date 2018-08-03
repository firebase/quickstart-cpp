Firebase Cloud Functions Quickstart
========================

The Firebase Cloud Functions Test Application (testapp) demonstrates Firebase
Cloud Functions operations with the Firebase Cloud Functions C++ SDK. The
application has no user interface and simply logs actions it's performing to the
console.

The testapp performs the following:
  - Creates a firebase::App in a platform-specific way. The App holds
    platform-specific context that's used by other Firebase APIs, and is a
    central point for communication between the Firebase Cloud Functions C++
    and Firebase Auth C++ libraries.
  - Creates a Functions Object
  - Grabs a HTTPS function reference to 'echoBody'
  - Sends various values to server, and checks that the echo response
    is the same as what was sent.
  - Shuts down the Firebase functions and Firebase App systems.

The testserver performs the following:
  - Echos the request body back as a JSON response.

Requirements
------------

- [Please make sure that the Firebase CLI is installed on your machine](https://firebase.google.com/docs/cli/)


Building and Running the testsever
--------------------------------

First, create an empty folder.  From the command shell, execute the following:

```
$ firebase init
```

Follow the prompts and initialize a new Firebase project with support for Functions.  Be sure to select the same project that you used for testapp. (See testapp/readme.md)

Copy the `testserver\functions\index.js` to the `functions\index.js` in your project folder.  This will overwrite your current (empty) file.

Run the following command to deploy the functions:

```
$ firebase deploy
```

Read the `functions\index.js` file that you *just* deployed to make sure everything looks kosher.