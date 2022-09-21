/*
obs-websocket
Copyright (C) 2016-2017	Stéphane Lepin <stephane.lepin@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <obs-frontend-api.h>

#include <QObject>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QTime>
#include <QSystemTrayIcon>
#include <QMainWindow>
#include <QInputDialog>
#include <QMessageBox>

#define SECTION_NAME "WebsocketAPI"
#define PARAM_ENABLE "ServerEnabled"
#define PARAM_PORT "ServerPort"
#define PARAM_LOCKTOIPV4 "LockToIPv4"
#define PARAM_DEBUG "DebugEnabled"
#define PARAM_ALERT "AlertsEnabled"
#define PARAM_AUTHREQUIRED "AuthRequired"
#define PARAM_SECRET "AuthSecret"
#define PARAM_SALT "AuthSalt"

#define GLOBAL_AUTH_SETUP_PROMPTED "AuthSetupPrompted"

#include "Utils.h"
#include "WSServer.h"

#include "Config.h"

#define QT_TO_UTF8(str) str.toUtf8().constData()

Config::Config() :
	ServerPort(4444),
	LockToIPv4(true),
	DebugEnabled(true),
	AlertsEnabled(true),
	AuthRequired(false),
	Secret(""),
	Salt("")
{

	SessionChallenge = GenerateSalt();
}

Config::~Config()
{

}

QString Config::GenerateSalt()
{
	// Get OS seeded random number generator
	QRandomGenerator *rng = QRandomGenerator::global();

	// Generate 32 random chars
	const size_t randomCount = 32;
	QByteArray randomChars;
	for (size_t i = 0; i < randomCount; i++)
		randomChars.append((char)rng->bounded(255));

	// Convert the 32 random chars to a base64 string
	return randomChars.toBase64();
}

QString Config::GenerateSecret(QString password, QString salt)
{
	// Create challenge hash
	auto challengeHash = QCryptographicHash(QCryptographicHash::Algorithm::Sha256);
	// Add password bytes to hash
	challengeHash.addData(password.toUtf8());
	// Add salt bytes to hash
	challengeHash.addData(salt.toUtf8());

	// Generate SHA256 hash then encode to Base64
	return challengeHash.result().toBase64();
}

void Config::SetPassword(QString password)
{
	QString newSalt = GenerateSalt();
	QString newChallenge = GenerateSecret(password, newSalt);

	this->Salt = newSalt;
	this->Secret = newChallenge;
}

bool Config::CheckAuth(QString response)
{
	// Concatenate auth secret with the challenge sent to the user
	QString challengeAndResponse = "";
	challengeAndResponse += Secret;
	challengeAndResponse += SessionChallenge;

	// Generate a SHA256 hash of challengeAndResponse
	auto hash = QCryptographicHash::hash(
		challengeAndResponse.toUtf8(),
		QCryptographicHash::Algorithm::Sha256
	);

	// Encode the SHA256 hash to Base64
	QString expectedResponse = hash.toBase64();

	bool authSuccess = false;
	if (response == expectedResponse) {
		SessionChallenge = GenerateSalt();
		authSuccess = true;
	}

	return authSuccess;
}

