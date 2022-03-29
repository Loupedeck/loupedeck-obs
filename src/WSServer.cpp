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

#include <chrono>
#include <thread>

#include <iostream>
#include <fstream>

#include <QtCore/QThread>
#include <QtCore/QByteArray>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <obs-frontend-api.h>
#include <util/platform.h>

#include "WSServer.h"
#include "obs-websocket.h"
#include "Config.h"
#include "Utils.h"
#include "protocol/OBSRemoteProtocol.h"

QT_USE_NAMESPACE

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

WSServer::WSServer()
	: QObject(nullptr),
	  _connections(),
	  _clMutex(QMutex::Recursive)
{
		_server.get_alog().clear_channels(websocketpp::log::alevel::frame_header | websocketpp::log::alevel::frame_payload | websocketpp::log::alevel::control);
	_server.init_asio();
#ifndef _WIN32
	_server.set_reuse_addr(true);
#endif

	_server.set_open_handler(bind(&WSServer::onOpen, this, ::_1));
	_server.set_close_handler(bind(&WSServer::onClose, this, ::_1));
	_server.set_message_handler(bind(&WSServer::onMessage, this, ::_1, ::_2));
}

WSServer::~WSServer()
{
	stop();
}

void WSServer::serverRunner()
{
	blog(LOG_INFO, "IO thread started.");
	try {
		_server.run();
	} catch (websocketpp::exception const & e) {
		blog(LOG_ERROR, "websocketpp instance returned an error: %s", e.what());
	} catch (const std::exception & e) {
		blog(LOG_ERROR, "websocketpp instance returned an error: %s", e.what());
	} catch (...) {
		blog(LOG_ERROR, "websocketpp instance returned an error");
	}
	blog(LOG_INFO, "IO thread exited.");
}


#ifndef WIN32
#define ENV_PREFIX "HOME"
#define ENV_PATH "/.local/share/Loupedeck/PluginData/ObsStudio/"
#else
#define ENV_PREFIX "LOCALAPPDATA"
#define ENV_PATH "\\Loupedeck\\PluginData\\ObsStudio\\"
#endif

#define PORT_FILE "websocket.port"

std::string get_port_file_path(void)
{
	std::string dataFilePath = getenv(ENV_PREFIX);

	if (dataFilePath.length() == 0)
		throw std::runtime_error("Cannot get home directory path");

	dataFilePath += ENV_PATH;
	dataFilePath += PORT_FILE;

	return dataFilePath;
}

unsigned short get_free_port(void)
{
	unsigned short port(0);
	try {
		/* https://stackoverflow.com/a/19923459 */
		asio::io_service service;
		asio::ip::tcp::acceptor acceptor(service);
		/* https://stackoverflow.com/questions/34562824 */
		asio::ip::tcp::endpoint endPoint(asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port));

		acceptor.open(endPoint.protocol());
		acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		acceptor.bind(endPoint);

		acceptor.listen();

		asio::ip::tcp::endpoint le = acceptor.local_endpoint();
		port = le.port();
		//acceptor.close() will be called on exit of this block
		blog(LOG_INFO, "using port %d", port);
	}
	catch (std::exception& ex) {
		blog(LOG_ERROR, "Cannot obtain a free port! Exception: %s", ex.what());
	}

	return port;
}

void remove_port_file(void)
{
	std::string fname = get_port_file_path();
	if (!std::remove(fname.c_str()))
	{
		blog(LOG_ERROR, "Cannot remove port file %s", fname.c_str());
	}
}

bool save_port_to_file(unsigned short port)
{
	try {
	        remove_port_file();

		std::string fname = get_port_file_path();
		std::ofstream df(fname, std::ofstream::out);

		blog(LOG_INFO, "Will save to file %s", fname.c_str());
		if (!df.is_open()) {
			blog(LOG_INFO, "Error %s saving to file %s", strerror(errno), fname.c_str());
			throw std::runtime_error(strerror(errno));
		}
		df << port;
		df.close();
	}
	catch (std::exception& ex) {
		blog(LOG_ERROR, "Cannot save Loupedeck port to file! Exception: %s", ex.what());
		return false;
	}

	return true;
}

void WSServer::start(quint16 port, bool lockToIPv4)
{
	//Note that in LD plugin case, server is started once during module load. 
	//There is no code path leading to restarting the server mid-flight. 

	if (_server.is_listening() && (port == _serverPort && _lockToIPv4 == lockToIPv4)) {
		blog(LOG_INFO, "WSServer::start: server already on this port and protocol mode. no restart needed");
		return;
	}

	if (_server.is_listening()) {
		stop();
	}

	_server.reset();

	//LD Plugin case: port will be used only as a fallback;  We use dynamically allocated port.
	_serverPort = get_free_port();

	if ( _serverPort == 0 ) {
		blog(LOG_ERROR, "WSServer::start: Cannot use dynamic port with Loupedeck. Using fallback port %d instead",port);
		_serverPort = port;
	}

	_lockToIPv4 = lockToIPv4;

	websocketpp::lib::error_code errorCode;
	if (lockToIPv4) {
		blog(LOG_INFO, "WSServer::start: Locked to IPv4 bindings");
		// listening only on localhost
		_server.listen(websocketpp::lib::asio::ip::address::from_string("127.0.0.1"), _serverPort, errorCode);
	} else {
		blog(LOG_INFO, "WSServer::start: Not locked to IPv4 bindings");
		//Also listening on all interfaces.
		_server.listen(_serverPort, errorCode);
	}

	if (errorCode) {
		std::string errorCodeMessage = errorCode.message();
		blog(LOG_INFO, "server: listen failed: %s", errorCodeMessage.c_str());

		obs_frontend_push_ui_translation(obs_module_get_string);
		QString errorTitle = "Loupedeck Connector failure";
        	QString errorMessage = QString("Loupedeck Connector failed to start.\n"
					       "Please check https://support.loupedeck.com for possible solutions\n"
					       "The error message: \"%2\"").arg(errorCodeMessage.c_str());
		obs_frontend_pop_ui_translation();

		QMainWindow* mainWindow = reinterpret_cast<QMainWindow*>(obs_frontend_get_main_window());
		QMessageBox::warning(mainWindow, errorTitle, errorMessage);

		return;
	}

	_server.start_accept();

	_serverThread = std::thread(&WSServer::serverRunner, this);

	blog(LOG_INFO, "server started successfully on port %d", _serverPort);

	if (!save_port_to_file(_serverPort)) {
		blog(LOG_ERROR, "WSServer::start: Cannot save dynamic port to Loupedeck!" );
	}


}

void WSServer::stop()
{
	auto config = GetConfig();
	if (config && config->DebugEnabled)
		blog(LOG_INFO, "[WSServer::stop] Stopping...");

	if (!_server.is_listening()) {
		if (config && config->DebugEnabled)
			blog(LOG_INFO, "[WSServer::stop] Server not listening.");
		return;
	}

	_server.stop_listening();

	//LD plugin: Removing port file if it exists
	remove_port_file();

	for (connection_hdl hdl : _connections) {
		if (config && config->DebugEnabled)
			blog(LOG_INFO, "[WSServer::stop] Closing an active connection...");
		websocketpp::lib::error_code errorCode;
		_server.pause_reading(hdl, errorCode);
		if (errorCode) {
			blog(LOG_ERROR, "Error: %s", errorCode.message().c_str());
			continue;
		}

		_server.close(hdl, websocketpp::close::status::going_away, "Server stopping", errorCode);
		if (errorCode) {
			blog(LOG_ERROR, "Error: %s", errorCode.message().c_str());
			continue;
		}
		if (config && config->DebugEnabled)
			blog(LOG_INFO, "[WSServer::stop] Finished closing connection.");
	}

	if (config && config->DebugEnabled)
		blog(LOG_INFO, "[WSServer::stop] Stopping thread pool...");
	_threadPool.waitForDone();

	if (config && config->DebugEnabled)
		blog(LOG_INFO, "[WSServer::stop] Waiting for all connections to close...");
	while (_connections.size() > 0)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

	if (config && config->DebugEnabled)
		blog(LOG_INFO, "[WSServer::stop] Performing join on IO thread...");
	_serverThread.join();

	blog(LOG_INFO, "Server stopped successfully.");
}

void WSServer::broadcast(const RpcEvent& event)
{
	std::string message = OBSRemoteProtocol::encodeEvent(event);

	auto config = GetConfig();
	if (config && config->DebugEnabled) {
		blog(LOG_INFO, "Update << '%s'", message.c_str());
	}

	QMutexLocker locker(&_clMutex);
	for (connection_hdl hdl : _connections) {
		if (config && config->AuthRequired) {
			bool authenticated = _connectionProperties[hdl].isAuthenticated();
			if (!authenticated) {
				continue;
			}
		}

		websocketpp::lib::error_code errorCode;
		_server.send(hdl, message, websocketpp::frame::opcode::text, errorCode);

		if (errorCode) {
			std::string errorCodeMessage = errorCode.message();
			blog(LOG_INFO, "server(broadcast): send failed: %s",
				errorCodeMessage.c_str());
		}
	}
}

bool WSServer::isListening()
{
	return _server.is_listening();
}

void WSServer::onOpen(connection_hdl hdl)
{
	QMutexLocker locker(&_clMutex);
	_connections.insert(hdl);
	locker.unlock();

	QString clientIp = getRemoteEndpoint(hdl);
	notifyConnection(clientIp);
	blog(LOG_INFO, "new client connection from %s", clientIp.toUtf8().constData());
}

void WSServer::onMessage(connection_hdl hdl, server::message_ptr message)
{
	auto opcode = message->get_opcode();
	if (opcode != websocketpp::frame::opcode::text) {
		return;
	}

	QtConcurrent::run(&_threadPool, [=]() {
		std::string payload = message->get_payload();

		QMutexLocker locker(&_clMutex);
		ConnectionProperties& connProperties = _connectionProperties[hdl];
		locker.unlock();

		auto config = GetConfig();
		if (config && config->DebugEnabled) {
			blog(LOG_INFO, "Request >> '%s'", payload.c_str());
		}

		WSRequestHandler requestHandler(connProperties);
		std::string response = OBSRemoteProtocol::processMessage(requestHandler, payload);

		if (config && config->DebugEnabled) {
			blog(LOG_INFO, "Response << '%s'", response.c_str());
		}

		websocketpp::lib::error_code errorCode;
		_server.send(hdl, response, websocketpp::frame::opcode::text, errorCode);

		if (errorCode) {
			std::string errorCodeMessage = errorCode.message();
			blog(LOG_INFO, "server(response): send failed: %s",
				errorCodeMessage.c_str());
		}
	});
}

void WSServer::onClose(connection_hdl hdl)
{
	QMutexLocker locker(&_clMutex);
	_connections.erase(hdl);
	_connectionProperties.erase(hdl);
	locker.unlock();

	auto conn = _server.get_con_from_hdl(hdl);
	auto localCloseCode = conn->get_local_close_code();
	auto localCloseReason = conn->get_local_close_reason();
	QString clientIp = getRemoteEndpoint(hdl);

	blog(LOG_INFO, "Websocket connection with client '%s' closed (disconnected). Code is %d, reason is: '%s'", clientIp.toUtf8().constData(), localCloseCode, localCloseReason.c_str());
	if (localCloseCode != websocketpp::close::status::going_away && _server.is_listening()) {
		notifyDisconnection(clientIp);
	}
}

QString WSServer::getRemoteEndpoint(connection_hdl hdl)
{
	auto conn = _server.get_con_from_hdl(hdl);
	return QString::fromStdString(conn->get_remote_endpoint());
}

void WSServer::notifyConnection(QString clientIp)
{
	//AL: TODO: Store connection status/data somewhere to be shown  from the settings dialog
	obs_frontend_push_ui_translation(obs_module_get_string);
	QString title = "Loupedeck Connector"; /*tr("OBSWebsocket.NotifyConnect.Title");*/
	QString msg = "Loupedeck connected"; /*tr("OBSWebsocket.NotifyConnect.Message").arg(clientIp);*/
	obs_frontend_pop_ui_translation();

	Utils::SysTrayNotify(msg, QSystemTrayIcon::Information, title);
}

void WSServer::notifyDisconnection(QString clientIp)
{
	obs_frontend_push_ui_translation(obs_module_get_string);
	QString title = "Loupedeck Connector";  /*tr("OBSWebsocket.NotifyDisconnect.Title");*/
	QString msg = "Loupedeck disconnected"; /*tr("OBSWebsocket.NotifyDisconnect.Message").arg(clientIp);*/
	obs_frontend_pop_ui_translation();

	Utils::SysTrayNotify(msg, QSystemTrayIcon::Information, title);
}
