import 'ui/autoload/modules';
var module = require('ui/modules').get('kibana/cast_search',['kibana']);
module.controller('elSearch', function($scope, es){
    console.log("boku no hero");
    var elastic = es;
    console.log(es);
    es.ping().then(function(body){ console.log(body);console.log("PING");});

    $scope.search = function(job) {
        console.log("scope");
        console.log($scope);
        console.log("el");
        console.log(elastic.indices);
        

        var query = '{ "query" : { "bool" : { "should" : [ { "match": { "data.allocation_id" : ' + 
            job.aid +  " } } ] } } }";
        console.log(query);

        elastic.search({
            index: "cast-allocation",
            body: query
        }).then(function(body){
            console.log(body.hits.hits);
        });
    }

});
