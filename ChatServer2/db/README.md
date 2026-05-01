# ChatServer 数据库脚本

`New-Client/ChatServer` 代码在查询 `user` 表时会读取这些字段：

- `uid`, `name`, `email`, `pwd`, `nick`, `desc`, `sex`, `icon`

如果你当前数据库的 `user` 表字段和上面不一致（例如只有 `id/passwd`），会导致登录阶段查询用户信息时报错。

## 一键修复（推荐）

在 MySQL 客户端中执行：

```sql
USE cjh; -- 换成你自己的 schema
SOURCE mysql_user_migration.sql;
```

脚本路径：`llfc_chat_project/New-Client/ChatServer/db/mysql_user_migration.sql`

