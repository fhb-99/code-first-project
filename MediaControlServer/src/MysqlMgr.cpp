#include "MysqlMgr.h"

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

int MysqlMgr::RegisterUser(const std::string &name, const std::string &email, const std::string &pwd)
{
    auto con = pool_->getConnection();
    try
    {
        if (con == nullptr)
        {
            pool_->returnConnection(std::move(con));
            return false;
        }
        // 准备调用存储过程
        std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
        // 设置输入参数
        stmt->setString(1, name);
        stmt->setString(2, email);
        stmt->setString(3, pwd);

        // 由于PreparedStatement不直接支持注册输出参数，我们需要使用会话变量或其他方法来获取输出参数的值

        // 执行存储过程
        stmt->execute();
        // 如果存储过程设置了会话变量或有其他方式获取输出参数的值，你可以在这里执行SELECT查询来获取它们
        // 例如，如果存储过程设置了一个会话变量@result来存储输出结果，可以这样获取：
        std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
        if (res->next())
        {
            int result = res->getInt("result");
            std::cout << "Result: " << result << std::endl;
            pool_->returnConnection(std::move(con));
            return result;
        }
        pool_->returnConnection(std::move(con));
        return -1;
    }
    catch (sql::SQLException &e)
    {
        pool_->returnConnection(std::move(con));
        std::cerr << "SQLException: " << e.what();
        std::cerr << " (MySQL error code: " << e.getErrorCode();
        std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
        return -1;
    }
}

bool MysqlMgr::CheckEmail(const std::string &name, const std::string &email)
{
    auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

		// 绑定参数
		pstmt->setString(1, name);

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		// 遍历结果集
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				pool_->returnConnection(std::move(con));
				return false;
			}
			pool_->returnConnection(std::move(con));
			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlMgr::UpdatePwd(const std::string &name, const std::string &newpwd)
{
    auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

		// 绑定参数
		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);

		// 执行更新
		int updateCount = pstmt->executeUpdate();

		std::cout << "Updated rows: " << updateCount << std::endl;
		pool_->returnConnection(std::move(con));
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlMgr::CheckPwd(const std::string &email, const std::string &pwd, UserInfo &userInfo)
{
    auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, email); // 将username替换为你要查询的用户名

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if (!res->next()) {
			return false;
		}

		const std::string origin_pwd = res->getString("pwd");
		// 输出查询到的密码
		std::cout << "Password: " << origin_pwd << std::endl;

		if (pwd != origin_pwd) {
			return false;
		}
		userInfo.name = res->getString("name");
		userInfo.email = res->getString("email");
		userInfo.uid = res->getInt("uid");
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}



std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return nullptr;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));
		pstmt->setInt(1, uid);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user = nullptr;

		while(res->next()) 
		{
			user.reset(new UserInfo);
			user->pwd = res->getString("pwd");
			user->email = res->getString("email");
			user->name= res->getString("name");
			user->nick = res->getString("nick");
			user->desc = res->getString("desc");
			user->sex = res->getInt("sex");
			user->icon = res->getString("icon");
			user->uid = uid;
			break;
		}
		return user;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}



std::shared_ptr<UserInfo> MysqlMgr::GetUser(std::string name)
{
	auto con = pool_->getConnection();
	if(con == nullptr)
	{
		return nullptr;
	}

	Defer defer([this, &con](){
		pool_->returnConnection(std::move(con));
	});

	try
	{
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, name);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user = nullptr;

		while(res->next()) 
		{
			user.reset(new UserInfo);
			user->pwd = res->getString("pwd");
			user->email = res->getString("email");
			user->nick = res->getString("nick");
			user->desc = res->getString("desc");
			user->sex = res->getInt("sex");
			user->icon = res->getString("icon");
			user->uid = res->getInt("uid");
			user->name = name;
			break;
		}
		return user;
	}
	catch(sql::SQLException& e)
	{
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}



bool MysqlMgr::AddFriendApply(const int& from, const int& to)
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
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT INTO friend_apply (from_id, to_uid) values (?,?) "
			"ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid"));

		pstmt->setInt(1, from);
		pstmt->setInt(2, to);

		int rowAffected = pstmt->executeUpdate();
		if(rowAffected < 0)
		{
			return false;
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



bool MysqlMgr::AuthFriendApply(const int& from, const int& to)
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
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
			"WHERE from_uid = ? AND to_uid = ?"));

		pstmt->setInt(1, to);
		pstmt->setInt(2, from);

		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) 
		{
			return false;
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




bool MysqlMgr::AddFriend(const int& from, const int& to, std::string back_name)
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
		con->_con->setAutoCommit(false);

		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "));

		pstmt->setInt(1, from);
		pstmt->setInt(2, to);
		pstmt->setString(3, back_name);

		int rowAffected = pstmt->executeUpdate();
		if(rowAffected < 0)
		{
			con->_con->rollback();
			return false;
		}


		std::unique_ptr<sql::PreparedStatement> pstmt2(con->_con->prepareStatement("INSERT IGNORE INTO friend(self_id, friend_id, back) "
			"VALUES (?, ?, ?) "));

		pstmt->setInt(1, from); // from id
		pstmt->setInt(2, to);
		pstmt->setString(3, back_name);

		int rowAffected2 = pstmt->executeUpdate();
		if(rowAffected2 < 0)
		{
			con->_con->rollback();
			return false;
		}

		con->_con->commit();
		std::cout << "addfriend insert friends success" << std::endl;

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




bool MysqlMgr::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int offset, int limit)
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
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select apply.from_uid, apply.status, user.name, "
				"user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
			"and apply.id > ? order by apply.id ASC LIMIT ? "));

		pstmt->setInt(1, touid); 
		pstmt->setInt(2, offset); 
		pstmt->setInt(3, limit);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) 
		{	
			auto name = res->getString("name");
			auto uid = res->getInt("from_uid");
			auto status = res->getInt("status");
			auto nick = res->getString("nick");
			auto sex = res->getInt("sex");
			auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
			applyList.push_back(apply_ptr);
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




bool MysqlMgr::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list)
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
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select * from friend where self_id = ? "));

		pstmt->setInt(1, self_id);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		while (res->next()) 
		{		
			auto friend_id = res->getInt("friend_id");
			auto back = res->getString("back");
			
			auto user_info = GetUser(friend_id);
			if (user_info == nullptr) {
				continue;
			}

			user_info->back = user_info->name;
			user_info_list.push_back(user_info);
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
		std::unique_ptr<sql::PreparedStatement> pstmt(
			con->_con->prepareStatement(
				"SELECT id, stream_id, name, url, source_type, status, owner_uid AS owner_id "
				"FROM media_stream WHERE owner_uid = ? AND status = 1 ORDER BY id ASC"));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next())
		{
			auto media_info = std::make_shared<MediaListInfo>();
			media_info->id = static_cast<int>(res->getUInt64("id"));
			media_info->stream_id = res->getString("stream_id");
			media_info->name = res->getString("name");
			media_info->url = res->getString("url");
			media_info->source_type = res->getInt("source_type");
			media_info->status = res->getInt("status");
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


bool MysqlMgr::GetSessionInfo(int uid, std::string& session_id, std::string& session_name)
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
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT session_id, session_name FROM session WHERE uid = ?"));
		pstmt->setInt(1, uid);
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		if(res->next())
		{
			session_id = res->getString("stream_id");
			session_name = res->getString("session_name");
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
