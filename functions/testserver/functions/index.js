const functions = require('firebase-functions');

///////////////////////////////////////////////////////////////////////
// debug callback for testing Cloud Functions Calls
exports.echoBody = functions.https.onCall((data, context) => {
    // Whatever you do, DO NOT RETURN response UNTIL you have finished your work.
    // Wait on Promises with 'then' clauses if you need to.
    return data;
});