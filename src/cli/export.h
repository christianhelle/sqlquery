
#ifndef EXPORT_H
#define EXPORT_H

#include "../database/connectioninfo.h"
#include "../database/dbexportdata.h"

class Export {
public:
    static void exportDataToCsvFile(const ConnectionInfo &connection,
                                    const QString &outputFolder,
                                    bool showProgress);
};

#endif // EXPORT_H
