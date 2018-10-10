const nodeCo = require('./build/Release/node-coroutine');

const Co = new nodeCo.Coroutine();

function realCallback(fn){

    return function(){
        var arg = [...arguments];
        var gen, ret;
        gen = fn.apply(global,arg);
        while(1){
            ret = gen.next();
            if(ret.done){
                Co.done(ret.value.yield);
                Co.swap();
                return;
            }else{
                if(!ret.value || !ret.value.yield){
                    Co.swap();
                }else{
                    Co.swap(ret.value.yield)
                }

            }
        }

    }
}

function create(fn){
    if(fn.constructor.name != "GeneratorFunction"){
        fn = function*(params){
            fn.apply(global,params);
            yield;
        }
    }
    fn = realCallback(fn);
    var CoYield = Co.create(fn);
    retFn = function(){
        var arg = [...arguments];
        CoYield.arguments = arg;
        Co.swap(CoYield);
    }
    retFn.yield = CoYield;
    return retFn;
}

function destroy(fn){
    Co.destroy(fn.yield);
}

module.exports = {
    create: create,
    destroy: destroy
}