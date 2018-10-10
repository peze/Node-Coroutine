const co = require("../index");
var arr = [];
function sleep(time){
    var now = Date.now();
    while((now + time * 200) > Date.now()){

    }
}

var coustomer = co.create(function *(){
    while(1){
        var i = arr.pop();
        console.log("coustomer:" + i);
        sleep(1);
        yield producer;
    }
})

var producer = co.create(function *(){
    var i = 0;
    while(1){
        console.log("producer:" + i);
        arr.push(i++);
        sleep(1);
        yield coustomer;
    }
})




producer();

setTimeout(function(){
    console.log("timer")
},2000)

setImmediate(function(){
    console.log("immediate")
})