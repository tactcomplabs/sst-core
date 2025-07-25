// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/statapi/statoutputcsv.h"

#include "sst/core/stringize.h"

namespace SST::Statistics {

StatisticOutputCSV::StatisticOutputCSV(Params& outputParameters) :
    StatisticFieldsOutput(outputParameters)
{
    m_useCompression = outputParameters.find<bool>("compressed");
    // Announce this output object's name
    Output& out      = getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutputCSV enabled...\n");
    setStatisticOutputName("StatisticOutputCSV");
}

bool
StatisticOutputCSV::checkOutputParameters()
{
    // Get the parameters
    m_Separator       = getOutputParameters().find<std::string>("separator", ", ");
    m_FilePath        = getOutputParameters().find<std::string>("filepath", "StatisticOutput.csv");
    m_outputTopHeader = getOutputParameters().find<bool>("outputtopheader", true);
    m_outputSimTime   = getOutputParameters().find<bool>("outputsimtime", true);
    m_outputRank      = getOutputParameters().find<bool>("outputrank", true);

    // Perform some checking on the parameters
    if ( 0 == m_Separator.length() ) {
        // Separator is zero length
        return false;
    }
    if ( 0 == m_FilePath.length() ) {
        // Filepath is zero length
        return false;
    }

    return true;
}

void
StatisticOutputCSV::startOfSimulation()
{
    StatisticFieldInfo*        statField;
    std::string                outputBuffer;
    FieldInfoArray_t::iterator it_v;

    // Open the finalized filename
    if ( !openFile() ) return;

    // Initialize the OutputBufferArray with std::string objects
    for ( FieldInfoArray_t::iterator it_v = getFieldInfoArray().begin(); it_v != getFieldInfoArray().end(); it_v++ ) {
        m_OutputBufferArray.push_back(std::string(""));
    }

    if ( true == m_outputTopHeader ) {
        // Add a Component Time Header to the front
        outputBuffer = "ComponentName";
        outputBuffer += m_Separator;
        print("%s", outputBuffer.c_str());

        outputBuffer = "StatisticName";
        outputBuffer += m_Separator;
        print("%s", outputBuffer.c_str());

        outputBuffer = "StatisticSubId";
        outputBuffer += m_Separator;
        print("%s", outputBuffer.c_str());

        outputBuffer = "StatisticType";
        outputBuffer += m_Separator;
        print("%s", outputBuffer.c_str());

        if ( true == m_outputSimTime ) {
            // Add a Simulation Time Header to the front
            outputBuffer = "SimTime";
            outputBuffer += m_Separator;
            print("%s", outputBuffer.c_str());
        }

        if ( true == m_outputRank ) {
            // Add a rank Header to the front
            outputBuffer = "Rank";
            outputBuffer += m_Separator;
            print("%s", outputBuffer.c_str());
        }

        // Output all Headers
        it_v = getFieldInfoArray().begin();

        while ( it_v != getFieldInfoArray().end() ) {
            statField    = *it_v;
            outputBuffer = statField->getFieldName();
            outputBuffer += ".";
            outputBuffer += getFieldTypeShortName(statField->getFieldType());

            // Increment the iterator
            it_v++;
            // If not the last field, tack on a separator
            if ( it_v != getFieldInfoArray().end() ) {
                outputBuffer += m_Separator;
            }

            print("%s", outputBuffer.c_str());
        }
        print("\n");
    }
}

void
StatisticOutputCSV::endOfSimulation()
{
    // Close the file
    closeFile();
}

void
StatisticOutputCSV::implStartOutputEntries(StatisticBase* statistic)
{
    // Save the current Component and Statistic Names for when we stop output and send to file
    m_currentComponentName  = statistic->getCompName();
    m_currentStatisticName  = statistic->getStatName();
    m_currentStatisticSubId = statistic->getStatSubId();
    m_currentStatisticType  = statistic->getStatTypeName();

    // Starting Output, Initialize the Buffers
    for ( uint32_t x = 0; x < getFieldInfoArray().size(); x++ ) {
        // Initialize the Output Buffer Array strings
        m_OutputBufferArray[x] = "0";
    }
}

void
StatisticOutputCSV::implStopOutputEntries()
{
    uint32_t x;

    // Output the Component and Statistic names
    print("%s", m_currentComponentName.c_str());
    print("%s", m_Separator.c_str());
    print("%s", m_currentStatisticName.c_str());
    print("%s", m_Separator.c_str());
    print("%s", m_currentStatisticSubId.c_str());
    print("%s", m_Separator.c_str());
    print("%s", m_currentStatisticType.c_str());
    print("%s", m_Separator.c_str());

    // Done with Output, Send a line of data to the file
    if ( true == m_outputSimTime ) {
        // Add the Simulation Time to the front
        print("%" PRIu64, getCurrentSimCycle());
        print("%s", m_Separator.c_str());
    }

    // Done with Output, Send a line of data to the file
    if ( true == m_outputRank ) {
        // Add the Simulation Time to the front
        print("%d", getRank().rank);
        print("%s", m_Separator.c_str());
    }

    x = 0;
    while ( x < m_OutputBufferArray.size() ) {
        print("%s", m_OutputBufferArray[x].c_str());
        x++;
        if ( x != m_OutputBufferArray.size() ) {
            print("%s", m_Separator.c_str());
        }
    }
    print("\n");
}

void
StatisticOutputCSV::outputField(fieldHandle_t fieldHandle, int32_t data)
{
    std::string buffer;
    buffer                           = format_string("%" PRId32, data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void
StatisticOutputCSV::outputField(fieldHandle_t fieldHandle, uint32_t data)
{
    std::string buffer;
    buffer                           = format_string("%" PRIu32, data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void
StatisticOutputCSV::outputField(fieldHandle_t fieldHandle, int64_t data)
{
    std::string buffer;
    buffer                           = format_string("%" PRId64, data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void
StatisticOutputCSV::outputField(fieldHandle_t fieldHandle, uint64_t data)
{
    std::string buffer;
    buffer                           = format_string("%" PRIu64, data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void
StatisticOutputCSV::outputField(fieldHandle_t fieldHandle, float data)
{
    std::string buffer;
    buffer                           = format_string("%f", data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

void
StatisticOutputCSV::outputField(fieldHandle_t fieldHandle, double data)
{
    std::string buffer;
    buffer                           = format_string("%f", data);
    m_OutputBufferArray[fieldHandle] = buffer;
}

bool
StatisticOutputCSV::openFile()
{
    std::string filename = m_FilePath;
    // Set Filename with Rank if Num Ranks > 1
    if ( 1 < getNumRanks().rank ) {
        int         rank    = getRank().rank;
        std::string rankstr = "_" + std::to_string(rank);

        // Search for any extension
        size_t index = m_FilePath.find_last_of(".");
        if ( std::string::npos != index ) {
            // We found a . at the end of the file, insert the rank string
            filename.insert(index, rankstr);
        }
        else {
            // No . found, append the rank string
            filename += rankstr;
        }
    }

    // Get the absolute path for output file
    filename = getAbsolutePathForOutputFile(filename);


    if ( m_useCompression ) {
#ifdef HAVE_LIBZ
        m_gzFile = gzopen(filename.c_str(), "w");
        if ( nullptr == m_gzFile ) {
            // We got an error of some sort
            Output out = getSimulationOutput();
            out.fatal(CALL_INFO, 1, " : StatisticOutputCompressedCSV - Problem opening File %s - %s\n",
                m_FilePath.c_str(), strerror(errno));
            return false;
        }
#else
        return false;
#endif
    }
    else {
        m_hFile = fopen(filename.c_str(), "w");
        if ( nullptr == m_hFile ) {
            // We got an error of some sort
            Output out = getSimulationOutput();
            out.fatal(CALL_INFO, 1, " : StatisticOutputCSV - Problem opening File %s - %s\n", m_FilePath.c_str(),
                strerror(errno));
            return false;
            ;
        }
    }
    return true;
}

void
StatisticOutputCSV::closeFile()
{
    if ( m_useCompression ) {
#ifdef HAVE_LIBZ
        gzclose(m_gzFile);
#endif
    }
    else {
        fclose(m_hFile);
    }
}

int
StatisticOutputCSV::print(const char* fmt, ...)
{
    int     res = 0;
    va_list args;
    if ( m_useCompression ) {
#ifdef HAVE_LIBZ
#if ZLIB_VERBUM >= 0x1271
        /* zlib added gzvprintf in 1.2.7.1.  CentOS 7 apparently uses 1.2.7.0 */
        va_start(args, fmt);
        res = gzvprintf(m_gzFile, fmt, args);
        va_end(args);
#else
        ssize_t bufSize = 128;
        bool    retry   = true;
        do {
            char* buf = (char*)malloc(bufSize);

            va_start(args, fmt);
            ssize_t n = vsnprintf(buf, bufSize, fmt, args);
            va_end(args);

            if ( n < 0 ) {
                retry = false;
            }
            else if ( n < bufSize ) {
                gzprintf(m_gzFile, "%s", buf);
                /* Success */
                retry = false;
            }
            else {
                bufSize += 128;
            }
            free(buf);
        } while ( retry );

#endif
#endif
    }
    else {
        va_start(args, fmt);
        res = vfprintf(m_hFile, fmt, args);
        va_end(args);
    }
    return res;
}

void
StatisticOutputCSV::serialize_order(SST::Core::Serialization::serializer& ser)
{
    StatisticFieldsOutput::serialize_order(ser);
    SST_SER(m_Separator);
    SST_SER(m_FilePath);
    SST_SER(m_outputRank);
    SST_SER(m_outputSimTime);
    SST_SER(m_outputTopHeader);
    SST_SER(m_useCompression);
}

} // namespace SST::Statistics
