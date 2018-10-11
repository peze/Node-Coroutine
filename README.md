# Node-Coroutine
node协程模块，使用了libco的部分代码完成该模块.（目前还仍有崩溃问题，仍在寻求问题中）.

example:
	
	const co = require("../index");
    var arr = [];
    function sleep(time) {
        var now = Date.now();
        while (now + time * 200 > Date.now()) {}
    }

    var coustomer = co.create(function*() {
        while (1) {
            var i = arr.pop();
            console.log("coustomer:" + i);
            sleep(1);
            yield producer;
        }
    });

    var producer = co.create(function*() {
        var i = 0;
        while (1) {
            console.log("producer:" + i);
            arr.push(i++);
            sleep(1);
            yield coustomer;
        }
    });

    producer();
    
一个经典的生产者消费者的例子，producer和costomer中的循环并不会影响主线程的运行，并且可以通过yield决定调用的先后顺序，如果yeild后面没有值，则会在协程方法的池中找出最久一个没执行过的方法来执行。

不过目前会出现在做GC的时候崩溃的问题，在打了断点以后发现是出现了地址明显很奇怪的HeapObject，所以在取它信息的时候发生崩溃。详细信息见文章： [记一次node协程模块开发](http://blog.magicare.me/ji-yi-ci-nodexie-cheng-mo-kuai-kai-fa/)
