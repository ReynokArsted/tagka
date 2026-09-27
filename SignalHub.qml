import QtQuick

QtObject 
{
    signal selected(var id, string name)
    signal tagFilesRequested(var id, string name)
    signal tagDeleteRequested(var id, string name)
}