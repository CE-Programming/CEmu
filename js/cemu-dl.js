var CEmuDownloader = CEmuDownloader || {};

CEmuDownloader.config = {
    apiUrl: "https://oss.jfrog.org/artifactory/api",
    basePath: "/oss-snapshot-local/org/github/alberthdev/cemu/git/",
    baseDLPath: "https://oss.jfrog.org/artifactory/",
    lastPathSeen: null,
    running: false,
}

CEmuDownloader.setConfig = function(config) {
    for (var key in config) {
        CEmuDownloader.config[key] = config[key];
    }
}

CEmuDownloader.init = function() {
    ArtifactoryHunter.setConfig({
        apiUrl: CEmuDownloader.config.apiUrl
    });
};

CEmuDownloader.getLatestFolderHandler = function (callback, resp = null, curPath = null) {
    var dirList = [];
    console.log(resp)
    for (var fileOrFolder in resp) {
        if (resp[fileOrFolder]["type"] != "dir") {
            console.log("Returning curPath due to non-folder type on " + fileOrFolder);
            callback(curPath);
            return;
        }
        dirList.push(fileOrFolder);
    }
    
    dirList.sort();
    
    if (dirList.length > 0) {
        ArtifactoryHunter.listDirectory(
            resp[dirList[dirList.length - 1]]["path"],
            function(new_resp) {
                CEmuDownloader.getLatestFolderHandler(callback, new_resp, resp[dirList[dirList.length - 1]]["path"]);
            }
        );
    } else {
        console.error("CEmuDownloader failed to detect the latest folder.");
        $('#error-content').html("Failed to detect the latest folder. (Current path: " + curPath + ")<br />You may need to refresh this page in order to download again.");
        $('.modal').modal('close');
        $('#modal-error').modal('open');
    }
}

CEmuDownloader.getLatestFolder = function(callback, startingPath = null) {
    if (!startingPath) {
        startingPath = CEmuDownloader.config.basePath;
    }
    ArtifactoryHunter.listDirectory(startingPath, function(resp) {
        CEmuDownloader.getLatestFolderHandler(callback, resp, startingPath);
    });
};

CEmuDownloader.downloadRelease = function(arch, relType, buildType, callback) {
    $("#dl-btn-for-" + arch + "-bit-" + relType + "-" + buildType).attr("disabled", true);
    $("#dl-btn-for-" + arch + "-bit-" + relType + "-" + buildType).html("Downloading, please wait...");
    $("#progress-bar-for-" + arch + "-bit-" + relType + "-" + buildType).fadeIn();
    $("#progress-lbl-for-" + arch + "-bit-" + relType + "-" + buildType).html("Preparing your download...");
    $("#progress-for-" + arch + "-bit-" + relType + "-" + buildType).fadeIn();
    CEmuDownloader.getLatestFolder(function(final_path) {
        CEmuDownloader.downloadReleasePostLatest(final_path, arch, relType, buildType);
    });
};

CEmuDownloader.pathBasename = function (str) {
    var base = new String(str).substring(str.lastIndexOf('/') + 1); 
    return base;
};

CEmuDownloader.downloadReleasePostLatest = function(path, arch, relType, buildType) {
    /* File name is very straight forward:
     * FOLDER_NAME-win32-debug-shared.zip
     */
    console.log("[CEmuDownloader.downloadReleasePostLatest] Got final dir path: " + path);
    console.log("[CEmuDownloader.downloadReleasePostLatest] Got final dir name: " + CEmuDownloader.pathBasename(path));
    var finalDLPath = path + "/" + CEmuDownloader.pathBasename(path) + "-win" + arch + "-" + relType + "-" + (buildType == "static" ? "static" : "shared") + ".zip";
    $("#progress-lbl-for-" + arch + "-bit-" + relType + "-" + buildType).html("Almost there...");
    ArtifactoryHunter.identifyFile(finalDLPath, function(fileInfo) {
        CEmuDownloader.downloadReleasePostIdentify(finalDLPath, fileInfo, arch, relType, buildType);
    });
};

CEmuDownloader.bytesToSizeStr = function (bytes) {
   var sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
   if (bytes == 0) return '0 Byte';
   var i = parseInt(Math.floor(Math.log(bytes) / Math.log(1024)));
   return Math.round(bytes / Math.pow(1024, i), 2) + ' ' + sizes[i];
};

CEmuDownloader.isObjEmpty = function (obj) {
    for(var prop in obj) {
        if(obj.hasOwnProperty(prop))
            return false;
    }

    return JSON.stringify(obj) === JSON.stringify({});
};

CEmuDownloader.downloadReleasePostIdentify = function(finalRelPath, fileInfo, arch, relType, buildType) {
    var dlPath = null;
    if (CEmuDownloader.isObjEmpty(fileInfo)) {
        $("#file-lbl-for-" + arch + "-bit-" + relType + "-" + buildType).html("No file information was found. This might be a bug in the file fetching API - please report this to us!");
        dlPath = CEmuDownloader.config.baseDLPath + finalRelPath;
    } else {
        var strFileInfo = "<b>File:</b> " + CEmuDownloader.pathBasename(finalRelPath) + "<br />";
        strFileInfo += "<b>Size:</b> " + CEmuDownloader.bytesToSizeStr(fileInfo["size"]) + "<br />";
        strFileInfo += "<b>MD5:</b> " + fileInfo["md5"] + "<br />";
        strFileInfo += "<b>SHA1:</b> " + fileInfo["sha1"]+ "<br />";
        strFileInfo += "<b>Last Modified:</b> " + fileInfo["modifiedDate"] + "<br />";
        $("#file-lbl-for-" + arch + "-bit-" + relType + "-" + buildType).html(strFileInfo);
        dlPath = fileInfo["url"];
        console.log("dlPath configured from strFileInfo = " + dlPath);
    }
    
    $("#progress-lbl-for-" + arch + "-bit-" + relType + "-" + buildType).html(
        "Found it! Downloading...<br />(If the download does not start automatically, click <a href='" + dlPath + "'>here</a>!)"
    );
    
    setTimeout(function() {
        $("#dl-btn-for-" + arch + "-bit-" + relType + "-" + buildType).attr("disabled", false);
        $("#dl-btn-for-" + arch + "-bit-" + relType + "-" + buildType).html("Download");
        $("#progress-bar-for-" + arch + "-bit-" + relType + "-" + buildType).fadeOut();
        $("#progress-for-" + arch + "-bit-" + relType + "-" + buildType).fadeOut();
        $("#progress-lbl-for-" + arch + "-bit-" + relType + "-" + buildType).html("");
    }, 5000);
    
    window.location = dlPath;
    window.focus();
};

CEmuDownloader.initFileBrowserOnce = function() {
    CEmuDownloader.browseToDir();
    $("#fileBrowserTable").hide();
    $('#modal-browse').modal('open');
    CEmuDownloader.initFileBrowserOnce = function() { $('#modal-browse').modal('open'); };
};

CEmuDownloader.browseToDir = function(path = null) {
    if (!path) {
        path = CEmuDownloader.config.basePath;
    }
    $("#progress-bar-browse").fadeIn();
    $("#fileBrowserTable").fadeOut({duration: 100});
    ArtifactoryHunter.listDirectoryEnhanced(path, function(resp) {
        CEmuDownloader.browseToDirFetchedDirList(resp, path);
    });
};

CEmuDownloader.browseToDirFetchedDirList = function(resp, path) {
    var dirList = [], fileList = [];
    console.log(resp)
    for (var fileOrFolder in resp) {
        if (resp[fileOrFolder]["type"] == "dir") {
            dirList.push(fileOrFolder);
        } else {
            fileList.push(fileOrFolder);
        }
    }
    
    dirList.sort();
    fileList.sort();
    
    if (dirList.length + fileList.length > 0) {
        $("#browse-files-table").empty();
        
        for (var i = 0; i < dirList.length; i++) {
            $("#browse-files-table").append(
                "<tr><td><a class='waves-effect cemu-color-text' href='#' onclick='CEmuDownloader.browseToDir(\"" + resp[dirList[i]]["path"] + "\"); return false;'>" + 
                "<i class='material-icons left'>folder</i>" +
                dirList[i] +
                "</a></td><td>Folder</td><td><!-- Size --></td>" + 
                "<td><!-- Created --></td>" + 
                "<td><!-- Last Modified --></td>" + 
                "<td><!-- Info --></td></tr>");
        }
        
        for (var i = 0; i < fileList.length; i++) {
            $("#browse-files-table").append(
                "<tr><td>" + 
                "<a class='waves-effect cemu-color-text' href='" + resp[fileList[i]]["url"] + "'>" + 
                "<i class='material-icons left'>insert_drive_file</i>" + fileList[i] +
                "</a></td><td>File</td><td>" +
                CEmuDownloader.bytesToSizeStr(resp[fileList[i]]["size"]) +
                "</td>" + 
                "<td>" + resp[fileList[i]]["createdDate"] + "</td>" + 
                "<td>" + resp[fileList[i]]["modifiedDate"] + "</td>" + 
                "<td><a class='waves-effect cemu-color-text' href='#' onclick='" + 
                "CEmuDownloader.displayFileInfo(\"" + resp[fileList[i]]["path"] + "\"); return false;'>" +
                "<i class='material-icons left'>info</i></a></td></tr>");
        }
        
        /* Build breadcrumbs */
        $("#pathBreadCrumbs").empty();
        pathSplit = path.substr(CEmuDownloader.config.basePath.length).split("/");
        
        $("#pathBreadCrumbs").append("<a href='#' onclick='CEmuDownloader.browseToDir(\"" +
            CEmuDownloader.config.basePath + "\"); return false;' class='breadcrumb waves-effect waves-light'>Root</a>");
        
        var accumulatedPath = "";
        
        for (var i = 0; i < pathSplit.length; i++) {
            if (pathSplit[i] == "") continue;
            $("#pathBreadCrumbs").append("<a href='#' onclick='CEmuDownloader.browseToDir(\"" +
                CEmuDownloader.config.basePath + accumulatedPath + pathSplit[i] + "\"); return false;' class='breadcrumb waves-effect waves-light'>" + pathSplit[i] + "</a>");
            accumulatedPath += pathSplit[i] + "/";
        }
        
        $("#progress-bar-browse").fadeOut();
        $("#fileBrowserTable").fadeIn();
    } else {
        console.error("CEmuDownloader failed to list the folder.");
        $('#error-content').html("Failed to list the folder. (Current path: " + path + ")<br />You may need to refresh this page in order to download again.");
        $('.modal').modal('close');
        $('#modal-error').modal('open');
    }
};

CEmuDownloader.displayFileInfo = function(path) {
    ArtifactoryHunter.identifyFile(path, function(fileInfo) {
        CEmuDownloader.displayFileInfoPostFetch(path, fileInfo);
    });
};

CEmuDownloader.displayFileInfoPostFetch = function(path, fileInfo) {
    strFileInfo = "<h4>" + CEmuDownloader.pathBasename(path) + "</h4>";
    if (CEmuDownloader.isObjEmpty(fileInfo)) {
        strFileInfo += "No file information was found. This might be a bug in the file fetching API - please report this to us!";
    } else {
        strFileInfo += "<b>Size:</b> " + CEmuDownloader.bytesToSizeStr(fileInfo["size"]) + "<br />";
        strFileInfo += "<b>MIME Type:</b> " + fileInfo["mimeType"] + "<br />";
        strFileInfo += "<b>MD5:</b> " + fileInfo["md5"] + "<br />";
        strFileInfo += "<b>SHA1:</b> " + fileInfo["sha1"]+ "<br />";
        strFileInfo += "<b>Created By:</b> " + fileInfo["createdBy"] + "<br />";
        strFileInfo += "<b>Created On:</b> " + fileInfo["createdDate"] + "<br />";
        strFileInfo += "<b>Last Modified By:</b> " + fileInfo["modifiedBy"] + "<br />";
        strFileInfo += "<b>Last Modified On:</b> " + fileInfo["modifiedDate"] + "<br />";
    }
    $("#modal-browse-info .modal-content").html(strFileInfo);
    $("#modal-browse-info").modal('open');
};
