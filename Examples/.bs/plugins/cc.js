

Log(import.meta.name);

export const a = "asd";

var exports = {
    steps:[]
};

exports.steps.push({
    name:"Build",
    function: ()=>{
        Log("Called function")
    }
})

exports;