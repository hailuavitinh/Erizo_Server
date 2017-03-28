'use strict';
var userRegistry = require('./../mdb/userRegistry');

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('FUsersResource');

exports.login = function (req, res) {
	if (req.body.email === undefined) {
        log.info('message: login - invalid email');
        res.status(400).send('Empty email');
        return;
    }

    if (req.body.password === undefined) {
        log.info('message: login - invalid password');
        res.status(400).send('Empty password');
        return;
    }

    userRegistry.login(req.body.email, req.body.password, function(result) {
    	res.send(result);
    });

};