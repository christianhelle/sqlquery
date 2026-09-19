#ifndef RUN_H
#define RUN_H

#include <QString>

#include "../database/connectioninfo.h"


class Script {
public:
    static void executeSqlFile(const QString &sqlFilePath,
                               const ConnectionInfo &connection);
};

#endif //RUN_H
