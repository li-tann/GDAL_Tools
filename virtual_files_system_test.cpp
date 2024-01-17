#include <iostream>
#include <cpl_vsi_virtual.h>
#include <cpl_vsi.h>
#include <gdal_priv.h>

void CopyFileFromZipToDisk(const char* zipFilePath, const char* outputFile);

using namespace std;

int main(int argc, char* argv[])
{
    const char* zipFilePathIn = "/vsizip/D:/1_Data/ALOS2/180721-181110/0000281079_001001_ALOS2224680610-180721.zip/BRS-HV-ALOS2224680610-180721-FBDR1.1__A.jpg";
    const char* outputFilePathOut = "D:/1_Data/ALOS2/180721-181110/BRS-HV-ALOS2224680610-180721-FBDR1.1__A.jpg";

    VSILFILE* inFile = VSIFOpenL(zipFilePathIn, "r");
    if (!inFile)
    {
        CPLError(CE_Failure, CPLE_FileIO, "Failed to open file within ZIP: %s", zipFilePathIn);
        return -1;
    }

    // // 移动到文件末尾以获取文件大小
    // VSIFSeekL(inFile, 0, SEEK_END);
    // vsi_l_offset fileSize = VSIFTellL(inFile);

    // // 检查是否成功获取到文件大小
    // if (fileSize < 0 || VSIFEofL(inFile) != 0)
    // {
    //     CPLError(CE_Failure, CPLE_FileIO,
    //             "Failed to determine size of the file in ZIP: %s. Error: %s",
    //             zipFilePathIn, CPLGetLastErrorMsg());
    //     VSIFCloseL(inFile);
    //     return -5;
    // }
    // cout<<"filesize:"<<fileSize<<endl;
    // cout<<"inFile->Eof()"<<inFile->Eof()<<endl;
    // return -2;
    // 获取ZIP内文件的大小
    VSIFSeekL(inFile, 0, SEEK_END);
    vsi_l_offset fileSize = static_cast<vsi_l_offset>(VSIFTellL(inFile));
    if (fileSize == 0)
    {
        int nErrorNo = CPLGetLastErrorNo();
        if (nErrorNo != 0 && !VSIFEofL(inFile))
        {
            CPLError(CE_Failure, CPLE_FileIO,
                    "Failed to get size of the file in ZIP: %s. Error: %s",
                    zipFilePathIn, CPLGetLastErrorMsg());
        }
        else if (!VSIFEofL(inFile)) // 文件大小为0但未到文件末尾，这可能表示文件实际为空
        {
            CPLError(CE_Failure, CPLE_AppDefined, "The file in the ZIP is empty.");
        }

        VSIFCloseL(inFile);
        return -1;
    }
    VSIFSeekL(inFile, 0, SEEK_SET);
    cout<<"filesize:"<<fileSize<<endl;

    // 打开输出文件用于写入
    FILE* outFile = fopen(outputFilePathOut, "wb");
    if (!outFile)
    {
        CPLError(CE_Failure, CPLE_FileIO, "Failed to open output file: %s", outputFilePathOut);
        VSIFCloseL(inFile);
        return -1;
    }

    
    // 分配缓冲区以存储ZIP内文件的内容
    std::vector<char> buffer(fileSize);
    
    // 从ZIP文件读取数据
    size_t bytesRead = VSIFReadL(buffer.data(), 1, fileSize, inFile);
    if (bytesRead != fileSize)
    {
        CPLError(CE_Failure, CPLE_FileIO, "Failed to read complete file from ZIP.");
    }
    else
    {
        // 将数据写入到输出文件
        fwrite(buffer.data(), 1, bytesRead, outFile);
    }

    // 关闭输入和输出文件
    fclose(outFile);
    VSIFCloseL(inFile);


    // CopyFileFromZipToDisk(zipFilePathIn, outputFilePathOut);
    cout<<"CopyFileFromZipToDisk over"<<endl;
    return 0;
}

// 假设你想从ZIP中读取名为"my_file.txt"的文件，并复制到本地的"output.txt"


void CopyFileFromZipToDisk(const char* zipFilePath, const char* outputFile)
{
    // 使用VSIFOpenL打开ZIP内部的文件
    VSILFILE* inFile = VSIFOpenL(zipFilePath, "r");
    if (!inFile)
    {
        CPLError(CE_Failure, CPLE_FileIO, "Failed to open file within ZIP: %s", zipFilePath);
        return;
    }

    // 获取ZIP内文件的大小
    VSIFSeekL(inFile, 0, SEEK_END);
    vsi_l_offset fileSize = static_cast<vsi_l_offset>(VSIFTellL(inFile));
    if (fileSize == 0)
    {
        int nErrorNo = CPLGetLastErrorNo();
        if (nErrorNo != 0 && !VSIFEofL(inFile))
        {
            CPLError(CE_Failure, CPLE_FileIO,
                    "Failed to get size of the file in ZIP: %s. Error: %s",
                    zipFilePath, CPLGetLastErrorMsg());
        }
        else if (!VSIFEofL(inFile)) // 文件大小为0但未到文件末尾，这可能表示文件实际为空
        {
            CPLError(CE_Failure, CPLE_AppDefined, "The file in the ZIP is empty.");
        }

        VSIFCloseL(inFile);
        return;
    }
    VSIFSeekL(inFile, 0, SEEK_SET);
    cout<<"filesize:"<<fileSize<<endl;
    return;

    // 打开输出文件用于写入
    FILE* outFile = fopen(outputFile, "wb");
    if (!outFile)
    {
        CPLError(CE_Failure, CPLE_FileIO, "Failed to open output file: %s", outputFile);
        VSIFCloseL(inFile);
        return;
    }

    
    // 分配缓冲区以存储ZIP内文件的内容
    std::vector<char> buffer(fileSize);
    
    // 从ZIP文件读取数据
    size_t bytesRead = VSIFReadL(buffer.data(), 1, fileSize, inFile);
    if (bytesRead != fileSize)
    {
        CPLError(CE_Failure, CPLE_FileIO, "Failed to read complete file from ZIP.");
    }
    else
    {
        // 将数据写入到输出文件
        fwrite(buffer.data(), 1, bytesRead, outFile);
    }

    // 关闭输入和输出文件
    fclose(outFile);
    VSIFCloseL(inFile);
}
