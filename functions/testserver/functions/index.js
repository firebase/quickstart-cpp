const functions = require('firebase-functions');

///////////////////////////////////////////////////////////////////////
// debug callback for testing Cloud Functions Calls
exports.echoBody = functions.https.onRequest((request, response) => {
    // Whatever you do, DO NOT SEND response UNTIL you have finished your work.
    // Wait on Promises with 'then' clauses if you need to.
    response.status(200);
    response.json(request.body);
});