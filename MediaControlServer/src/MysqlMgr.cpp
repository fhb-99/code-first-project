#include "MysqlMgr.h"
#include "ConfigMgr.h"

MysqlMgr::MysqlMgr()
{
    auto & cfg = ConfigMgr::Inst();
    const auto& host = cfg["Mysql"]["Host"];
    const auto& port = cfg["Mysql"]["Port"];
    const auto& pwd = cfg["Mysql"]["Password"];
    const auto& schema = cfg["Mysql"]["Schema"];
    const auto& user = cfg["Mysql"]["User"];

	//mysql 必须加scheem 就像redis-plus-plus —— tcp://127.0.0.1:3306
	std::string url = host;
	if(url.find("://") == std::string::npos) 
	{
		url = "tcp://" + host + ":" + port;
	}
	else
	{
		const auto scheme_pos = url.find("://");
		const auto host_pos = (scheme_pos == std::string::npos) ? 0 : (scheme_pos + 3);
		if(url.find(':', host_pos) == std::string::npos)
		{
			url += ":" + port;
		}
	}

	std::cout << "MySQL url: " << url << "  scheme: " << schema << std::endl;
    pool_.reset(new MysqlPool(url, user, pwd, schema, 5));
}

MysqlMgr::~MysqlMgr()
{
    pool_->Close();
}

// int MysqlMgr::RegisterUser(const std::string &name, const std::string &email, const std::string &pwd)
// {
//     auto con = pool_->getConnection();
//     try
//     {
//         if (con == nullptr)
//         {
//             return -1;
//         }
//         // 准备调用存储过程
//         std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
//         // 设置输入参数
//         stmt->setString(1, name);
//         stmt->setString(2, email);
//         stmt->setString(3, pwd);

//         // 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值

//         // 执行存储过程
//         stmt->execute();
//         // 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
//         // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
//         std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
//         std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
//         if (res->next())
//         {
//             int result = res->getInt("result");
//             std::cout << "Result: " << result << std::endl;
//             pool_->returnConnection(std::move(con));
//             return result;
//         }
//         pool_->returnConnection(std::move(con));
//         return -1;
//     }
//     catch (sql::SQLException &e)
//     {
//         pool_->returnConnection(std::move(con));
//         std::cerr << "SQLException: " << e.what();
//         std::cerr << " (MySQL error code: " << e.getErrorCode();
//         std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
//         return -1;
//     }
// }

// bool MysqlMgr::CheckEmail(const std::string &name, const std::string &email)
// {
//     auto con = pool_->getConnection();
// 	try {
// 		if (con == nullptr) {
// 			return false;
// 		}

// 		// 准备查询语句
// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

// 		// 绑定参数
// 		pstmt->setString(1, name);

// 		// 执行查询
// 		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

// 		// 遍历结果集
// 		while (res->next()) {
// 			std::cout << "Check Email: " << res->getString("email") << std::endl;
// 			if (email != res->getString("email")) {
// 				pool_->returnConnection(std::move(con));
// 				return false;
// 			}
// 			pool_->returnConnection(std::move(con));
// 			return true;
// 		}
// 		pool_->returnConnection(std::move(con));
// 		return false;
// 	}
// 	catch (sql::SQLException& e) {
// 		pool_->returnConnection(std::move(con));
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return false;
// 	}
// }

// bool MysqlMgr::UpdatePwd(const std::string &name, const std::string &newpwd)
// {
//     auto con = pool_->getConnection();
// 	try {
// 		if (con == nullptr) {
// 			return false;
// 		}

// 		// 准备查询语句
// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

// 		// 绑定参数
// 		pstmt->setString(2, name);
// 		pstmt->setString(1, newpwd);

// 		// 执行更新
// 		const int updateCount = pstmt->executeUpdate();

// 		std::cout << "Updated rows: " << updateCount << std::endl;
// 		pool_->returnConnection(std::move(con));
// 		return updateCount > 0;
// 	}
// 	catch (sql::SQLException& e) {
// 		pool_->returnConnection(std::move(con));
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return false;
// 	}
// }

// bool MysqlMgr::CheckPwd(const std::string &email, const std::string &pwd, UserInfo &userInfo)
// {
//     auto con = pool_->getConnection();
// 	if (con == nullptr) {
// 		return false;
// 	}

// 	Defer defer([this, &con]() {
// 		pool_->returnConnection(std::move(con));
// 		});

// 	try {
// 		// 准备SQL语句
// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
// 		pstmt->setString(1, email); // 将username替换为你要查询的用户名

// 		// 执行查询
// 		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
// 		if (!res->next()) {
// 			return false;
// 		}

// 		const std::string origin_pwd = res->getString("pwd");
// 		// 输出查询到的密码
// 		std::cout << "Password: " << origin_pwd << std::endl;

// 		if (pwd != origin_pwd) {
// 			return false;
// 		}
// 		userInfo.name = res->getString("name");
// 		userInfo.email = res->getString("email");
// 		userInfo.uid = res->getInt("uid");
// 		userInfo.pwd = origin_pwd;
// 		return true;
// 	}
// 	catch (sql::SQLException& e) {
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return false;
// 	}
// }



// std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid)
// {
// 	auto con = pool_->getConnection();
// 	if(con == nullptr)
// 	{
// 		return nullptr;
// 	}

// 	Defer defer([this, &con](){
// 		pool_->returnConnection(std::move(con));
// 	});

// 	try
// 	{
// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
// 		pstmt->setInt(1, uid);

// 		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
// 		std::shared_ptr<UserInfo> user = nullptr;

// 		while(res->next()) 
// 		{
// 			user.reset(new UserInfo);
// 			user->pwd = res->getString("pwd");
// 			user->email = res->getString("email");
// 			user->name= res->getString("name");
// 			user->nick = res->getString("nick");
// 			user->desc = res->getString("desc");
// 			user->sex = res->getInt("sex");
// 			user->icon = res->getString("icon");
// 			user->uid = uid;
// 			break;
// 		}
// 		return user;
// 	}
// 	catch(sql::SQLException& e)
// 	{
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return nullptr;
// 	}
// }



// std::shared_ptr<UserInfo> MysqlMgr::GetUser(std::string name)
// {
// 	auto con = pool_->getConnection();
// 	if(con == nullptr)
// 	{
// 		return nullptr;
// 	}

// 	Defer defer([this, &con](){
// 		pool_->returnConnection(std::move(con));
// 	});

// 	try
// 	{
// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
// 		pstmt->setString(1, name);

// 		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
// 		std::shared_ptr<UserInfo> user = nullptr;

// 		while(res->next()) 
// 		{
// 			user.reset(new UserInfo);
// 			user->pwd = res->getString("pwd");
// 			user->email = res->getString("email");
// 			user->nick = res->getString("nick");
// 			user->desc = res->getString("desc");
// 			user->sex = res->getInt("sex");
// 			user->icon = res->getString("icon");
// 			user->uid = res->getInt("uid");
// 			user->name = name;
// 			break;
// 		}
// 		return user;
// 	}
// 	catch(sql::SQLException& e)
// 	{
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return nullptr;
// 	}
// }



// bool MysqlMgr::AddFriendApply(const int& from, const int& to)
// {
// 	auto con = pool_->getConnection();
// 	if(con == nullptr)
// 	{
// 		return false;
// 	}

// 	Defer defer([this, &con](){
// 		pool_->returnConnection(std::move(con));
// 	});

// 	try
// 	{
// 		// uk_friend_apply_from_to(from_id, to_uid)；from_uid 由触发器与 from_id 同步（见 db/mysql_social_migration.sql）
// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT INTO friend_apply (from_id, to_uid) VALUES (?,?) "
// 			"ON DUPLICATE KEY UPDATE updated_at = CURRENT_TIMESTAMP"));

// 		pstmt->setInt(1, from);
// 		pstmt->setInt(2, to);

// 		pstmt->executeUpdate();

// 		return true;
// 	}
// 	catch(sql::SQLException& e)
// 	{
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return false;
// 	}
	
// }



// bool MysqlMgr::AuthFriendApply(const int& from, const int& to)
// {
// 	auto con = pool_->getConnection();
// 	if (con == nullptr) {
// 		return false;
// 	}

// 	Defer defer([this, &con]() {
// 		pool_->returnConnection(std::move(con));
// 	});

// 	try
// 	{
// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
// 			"WHERE from_uid = ? AND to_uid = ?"));

// 		pstmt->setInt(1, from);
// 		pstmt->setInt(2, to);

// 		const int rowAffected = pstmt->executeUpdate();
// 		return rowAffected > 0;
// 	}
// 	catch(sql::SQLException& e)
// 	{
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return false;
// 	}
// }




// bool MysqlMgr::AddFriend(const int& from, const int& to, std::string back_name)
// {
// 	auto con = pool_->getConnection();
// 	if (con == nullptr) {
// 		return false;
// 	}

// 	Defer defer([this, &con]() {
// 		pool_->returnConnection(std::move(con));
// 	});

// 	try
// 	{
// 		con->_con->setAutoCommit(false);

// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
// 			"VALUES (?, ?, ?) "));

// 		pstmt->setInt(1, from);
// 		pstmt->setInt(2, to);
// 		pstmt->setString(3, back_name);

// 		pstmt->executeUpdate();

// 		std::unique_ptr<sql::PreparedStatement> pstmt2(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
// 			"VALUES (?, ?, ?) "));

// 		pstmt2->setInt(1, to);
// 		pstmt2->setInt(2, from);
// 		pstmt2->setString(3, back_name);

// 		pstmt2->executeUpdate();

// 		con->_con->commit();
// 		con->_con->setAutoCommit(true);
// 		std::cout << "addfriend insert friends success" << std::endl;

// 		return true;
// 	}
// 	catch(sql::SQLException& e)
// 	{
// 		try {
// 			con->_con->rollback();
// 			con->_con->setAutoCommit(true);
// 		} catch (sql::SQLException&) {
// 		}
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return false;
// 	}
// }




// bool MysqlMgr::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int offset, int limit)
// {
// 	auto con = pool_->getConnection();
// 	if (con == nullptr) {
// 		return false;
// 	}

// 	Defer defer([this, &con]() {
// 		pool_->returnConnection(std::move(con));
// 	});

// 	try
// 	{
// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select apply.from_uid, apply.status, user.name, "
// 				"user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
// 			"and apply.id > ? order by apply.id ASC LIMIT ? "));

// 		pstmt->setInt(1, touid); 
// 		pstmt->setInt(2, offset); 
// 		pstmt->setInt(3, limit);

// 		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

// 		while (res->next()) 
// 		{	
// 			auto name = res->getString("name");
// 			auto uid = res->getInt("from_uid");
// 			auto status = res->getInt("status");
// 			auto nick = res->getString("nick");
// 			auto sex = res->getInt("sex");
// 			auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
// 			applyList.push_back(apply_ptr);
// 		}
// 		return true;
// 	}
// 	catch(sql::SQLException& e)
// 	{
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return false;
// 	}
	
// }




// bool MysqlMgr::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list)
// {
// 	auto con = pool_->getConnection();
// 	if (con == nullptr) {
// 		return false;
// 	}

// 	Defer defer([this, &con]() {
// 		pool_->returnConnection(std::move(con));
// 	});

// 	try
// 	{
// 		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select * from friend where self_id = ? "));

// 		pstmt->setInt(1, self_id);

// 		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

// 		while (res->next()) 
// 		{		
// 			auto friend_id = res->getInt("friend_id");
// 			auto back = res->getString("back");
			
// 			auto user_info = GetUser(friend_id);
// 			if (user_info == nullptr) {
// 				continue;
// 			}

// 			user_info->back = back;
// 			user_info_list.push_back(user_info);
// 		}
		
// 		return true;
// 	}
// 	catch(sql::SQLException& e)
// 	{
// 		std::cerr << "SQLException: " << e.what();
// 		std::cerr << " (MySQL error code: " << e.getErrorCode();
// 		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
// 		return false;
// 	}
	
// }



bool MysqlMgr::GetMediaList(int uid, std::vector<std::shared_ptr<MediaListInfo>>& media_list)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try
	{
		// db/mysql_media_migration.sql：media_stream(stream_id, url, owner_id)
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT stream_id, url, owner_id FROM media_stream WHERE owner_id = ? ORDER BY stream_id ASC"));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next())
		{
			auto media_info = std::make_shared<MediaListInfo>();
			media_info->id = 0;      //暂时没有这个列
			media_info->stream_id = res->getString("stream_id");
			media_info->name = media_info->stream_id;
			media_info->url = res->getString("url");
			media_info->source_type = 0;
			media_info->status = 1;
			media_info->owner_id = res->getInt("owner_id");
			media_list.push_back(media_info);
		}
		return true;
	}
	catch (sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}


bool MysqlMgr::GetStreamInfo(int uid, std::string& stream_id)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return false;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT stream_id FROM media_stream WHERE owner_id = ? ORDER BY updated_at DESC LIMIT 1"));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if(res->next())
		{
			stream_id = res->getString("stream_id");
			return true;
		}
		return false;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}




bool MysqlMgr::UpdateMediaPlayStatus(int uid, const std::string& session_id, const std::string& stream_id, int state)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return false;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"UPDATE media_client_playing SET state = ? WHERE uid = ? AND session_id = ? AND stream_id = ?"));
		pstmt->setInt(1, state);
		pstmt->setInt(2, uid);
		pstmt->setString(3, session_id);
		pstmt->setString(4, stream_id);
		const int rowAffected = pstmt->executeUpdate();


		std::unique_ptr<sql::PreparedStatement> pstmt2(con->_con->prepareStatement(
			"UPDATE media_session_stream SET state = ? WHERE session_id = ? AND stream_id = ?"));
		pstmt2->setInt(1, state);
		pstmt2->setString(2, session_id);
		pstmt2->setString(3, stream_id);
		const int rowAffected2 = pstmt2->executeUpdate();

		return rowAffected > 0 || rowAffected2 > 0;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}



bool MysqlMgr::InsertMediaClientPlaying(int uid, const std::string& session_id, const std::string& stream_id, int state)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return false;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"INSERT INTO media_client_playing (session_id, uid, stream_id, state) VALUES (?, ?, ?, ?) "
			"ON DUPLICATE KEY UPDATE stream_id = VALUES(stream_id), state = VALUES(state)"));
		pstmt->setString(1, session_id);
		pstmt->setInt(2, uid);
		pstmt->setString(3, stream_id);
		pstmt->setInt(4, state);
		pstmt->executeUpdate();

		return true;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}


bool MysqlMgr::InsertMediaSessionStream(int uid, const std::string& session_id, const std::string& stream_id, int state)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return false;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"INSERT INTO media_session_stream (session_id, stream_id, owner_id, state) VALUES (?, ?, ?, ?) "
			"ON DUPLICATE KEY UPDATE owner_id = VALUES(owner_id), state = VALUES(state)"));
		pstmt->setString(1, session_id);
		pstmt->setString(2, stream_id);
		pstmt->setInt(3, uid);
		pstmt->setInt(4, state);
		pstmt->executeUpdate();

		return true;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}


//加个判断位，true，count+1，false，count-1
bool MysqlMgr::UpdateMediaSessionOnlineCount(int uid, const std::string& session_id, const std::string& stream_id, bool is_add)
{
	(void)uid;
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return false;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		if(is_add)
		{
			std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
				"UPDATE media_session_stream SET `count` = `count` + 1 WHERE session_id = ? AND stream_id = ?"));
			pstmt->setString(1, session_id);
			pstmt->setString(2, stream_id);
			pstmt->executeUpdate();
		}
		else
		{
			std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
				"UPDATE media_session_stream SET `count` = IF(`count` > 0, `count` - 1, 0) WHERE session_id = ? AND stream_id = ?"));
			pstmt->setString(1, session_id);
			pstmt->setString(2, stream_id);
			pstmt->executeUpdate();
		}
		return true;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}



bool MysqlMgr::GetSessionList(int uid, std::vector<std::shared_ptr<SessionInfo>>& session_list)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return false;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT session_id, owner_id, current_stream_id, state FROM media_session"
		));

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next())
		{
			auto session_info = std::make_shared<SessionInfo>();
			session_info->session_id = res->getString("session_id");
			session_info->owner_id = res->getInt("owner_id");
			session_info->current_stream_id = res->getString("current_stream_id");
			session_info->state = res->getInt("state");
			session_list.push_back(session_info);
		}
		return true;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}


bool MysqlMgr::IsStreamIDValid(const std::string& stream_id)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return false;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT COUNT(*) FROM media_stream WHERE stream_id = ?"
		));
		pstmt->setString(1, stream_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if(res->next())
		{
			return res->getInt(1) == 1;
		}
		return false;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}


std::string MysqlMgr::GetNameByUID(int uid)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return "";
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});
	
	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT name FROM user WHERE uid = ?"
		));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if(res->next())
		{
			return res->getString("name");
		}
		return "";
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return "";
	}
}


bool MysqlMgr::CreateSession(int uid, const std::string& session_id, const std::string& stream_id, int state)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return false;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		if(!IsStreamIDValid(stream_id))
		{
			return false;
		}

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"INSERT INTO media_session (session_id, session_name, owner_id, current_stream_id, state) VALUES (?, ?, ?, ?, ?)"
		));
		pstmt->setString(1, session_id);
		pstmt->setString(2, "房间" + std::to_string(uid) + "号");
		pstmt->setInt(3, uid);
		pstmt->setString(4, stream_id);
		pstmt->setInt(5, state);
		pstmt->executeUpdate();

		std::unique_ptr<sql::PreparedStatement> pstmt2(con->_con->prepareStatement(
			"INSERT INTO media_session_stream (session_id, stream_id, owner_id, state) VALUES (?, ?, ?, ?) "
			"ON DUPLICATE KEY UPDATE owner_id = VALUES(owner_id), state = VALUES(state)"
		));
		pstmt2->setString(1, session_id);
		pstmt2->setString(2, stream_id);
		pstmt2->setInt(3, uid);
		pstmt2->setInt(4, state);
		pstmt2->executeUpdate();

		return true;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}


int MysqlMgr::GetOwnerIDOfSession(const std::string& session_id)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return 0;
	}


	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT owner_id FROM media_session WHERE session_id = ?"
		));
		pstmt->setString(1, session_id);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if(res->next())
		{
			return res->getInt("owner_id");
		}
		return 0;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return 0;
	}

}


bool MysqlMgr::JoinSession(int uid, const std::string& session_id, const std::string& stream_id, int state)
{
	auto con = pool_->getConnection();
	if(con == nullptr) 
	{
		return false;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		const int owner_id = GetOwnerIDOfSession(session_id);
		if(owner_id == 0)
		{
			return false;
		}

		return InsertMediaClientPlaying(uid, session_id, stream_id, state);
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}