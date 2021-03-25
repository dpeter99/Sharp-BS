let defaultCompiler = "gcc";
let tempDir;
let defaultLinkFlags = "%.o"
let defaultOutput = "$name"

BSHost.RegisterBuildStep("compile",".c .h",compile);
BSHost.RegisterBuildStep("link",".c .h",link);
BSHost.RegisterBuildStep("output",".c .h",output);

BSHost.RegisterFileExtension(".c",extensionC);

function compile(params){
    let compileFlags = "";
    for(var param of params) {
        if(param.startsWith("-")) {
            compileFlags = param;
        } else {
            let filePath = param.substr(0, param.lastIndexOf("."));
            let fileName = param.substr(param.lastIndexOf("/"), param.lastIndexOf("."));
            let compilerCommand =
                defaultCompiler +
                " -c " + filePath + ".c" +
                " -o " + tempDir + "/" + fileName + ".o "
                + compileFlags;

            Exec(compilerCommand);
        }
    }
}

function extensionC(buildscript) {
    Log(buildscript.name)
    buildscript.name = "Something new";
    Log("c.js processing buildscript");
    if(!("compile" in buildscript.build.steps)) {
        Log("No build step found");
        //buildscript.Build.steps["link"] = [defaultLinkFlags];
    }

    if(!("link" in buildscript.build.steps)) {
        Log("Inserting link step for gcc");
        //buildscript.build.steps.Add({name:"link", args:[defaultLinkFlags]});
    }

    if(!("output" in buildscript.build.steps)) {
        Log("Inserting output step");
        buildscript.build.steps["output"] = [defaultOutput];
    }

    //tempDir = process.cwd() + "/bsTemp/";

    //if(!fs.existsSync(tempDir)) {
    //   Log("Creating temporary dir for object files");
    //   fs.mkdirSync(tempDir);
    //}
}

function link (...params) {
    let linkerCommand = "";
    // Check whether we're doubly listed
    if(Array.isArray(params[0])) {
        params = params[0];
    }

    for(var param of params) {
        console.log("stepLink: param", param);
        linkerCommand += param + " ";
    }

    linkCache = linkerCommand;
}

/**
 * Take in the name of a file, execute the link command to save the output.
 * @param {string} output the name of the wanted file
 */
function output (output) {
    // Check whether we're doubly listed
    if(Array.isArray(output[0])) {
        output = output[0];
    }

    //let linkerCommand = defaultCompiler + " -o " + process.cwd() + "/" + output + " " + linkCache;
    //console.log("stepOutput: param", output, "command", linkerCommand);
    //child.execSync(linkerCommand);

}

/**
 * Set the name of the compiler to be used.
 * @param {string} comp 
 */
function setCompiler (comp) {
    // Check whether we're doubly listed
    if(Array.isArray(comp[0])) {
        comp = comp[0];
    }
    defaultCompiler = comp;
    console.log("Set compiler to", comp);
}