/*global require, exports, ObjectId*/
'use strict';
var db = require('./dataBase').db;

var logger = require('./../logger').logger;

// Logger
var log = logger.getLogger('FUserRegistry');

exports.getUsers = function (callback) {
    db.users.find({}).toArray(function (err, users) {
        if (err || !users) {
            log.info('message: users list empty');
        } else {
            callback(users);
        }
    });
};

var getUserById = exports.getUserById = function (id, callback) {
    db.users.findOne({_id: db.ObjectId(id)}, function (err, user) {
        if (user === undefined) {
            log.warn('message: getuser - user not found, userId: ' + id);
        }
        if (callback !== undefined) {
            callback(user);
        }
    });
};

var getUserByEmail = exports.getUserByEmail = function (email, callback) {
    db.users.findOne({email: email}, function (err, user) {
        if (user === undefined) {
            log.warn('message: getuser - user not found, userId: ' + id);
        }
        if (callback !== undefined) {
            callback(user);
        }
    });
};

var hasUser = exports.hasUser = function (id, callback) {
    getUserById(id, function (user) {
        if (user === undefined) {
            callback(false);
        } else {
            callback(true);
        }
    });
};

// /*
//  * Adds a new user to the data base.
//  */
exports.addUser = function (user, callback) {

	db.users.findOne({email: user.email}, function (err, user) {
        if (user === undefined) {
            db.users.save(user, function (error, saved) {
		        if (error) log.warn('message: addUesr error, ' + logger.objectToLog(error));
		        callback(saved);
		    });
        }
        if (callback !== undefined) {
           log.warn('message: addUser - exit email');
           callback(false);
        }
    });

    
};

// /*
//  * Updates a determined room
//  */
exports.updateUser = function (id, user, callback) {
    db.users.update({_id: db.ObjectId(id)}, user, function (error) {
        if (error) log.warn('message: updateUser error, ' + logger.objectToLog(error));
        if (callback) callback(error);
    });
};

/*
 * Removes a determined room from the data base.
 */
exports.removeUser = function (id) {
    hasUser(id, function (hasU) {
        if (hasU) {
            db.users.remove({_id: db.ObjectId(id)}, function (error) {
                if (error) log.warn('message: removeUser error, ' +
                   logger.objectToLog(error));
            });
        }
    });
};

/* 
 * Login 
 */
 var login = exports.login = function (email, pass, callback) {
 	db.users.findOne({email: email, password: pass}, function (err, user) {
        if (user === undefined) {
             callback({ OK: false, Value: 'Not exit' });
        }
        if (callback !== undefined) {
            callback({ OK: true, Value: user });
        }
    });
};