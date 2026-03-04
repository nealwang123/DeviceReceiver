#ifndef DATAEXPORTER_H
#define DATAEXPORTER_H

#include <QString>
#include <QVector>
#include "FrameData.h"

class DataExporter
{
public:
    enum class Format {
        Csv,
        Hdf5
    };

    struct ExportOptions {
        QString filePath;
        QString sourceTag;
        qint64 startTimeMs = -1;
        qint64 endTimeMs = -1;
    };

    static bool exportFrames(const QVector<FrameData>& frames,
                             Format format,
                             const ExportOptions& options,
                             QString* errorMessage);
};

#endif // DATAEXPORTER_H
