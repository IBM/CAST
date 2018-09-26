import 'ui/autoload/modules';
//import { FilterManagerProvider } from 'ui/filter_manager';
import { FilterBarQueryFilterProvider } from 'ui/filter_bar/query_filter';
import { timefilter } from 'ui/timefilter';

var module = require('ui/modules').get('kibana/cast_search',['kibana']);
module.controller('castSearch', function($rootScope, $scope, es, Private, timefilter){
  //  var filterManager = Private(FilterManagerProvider);
    const queryFilter = Private(FilterBarQueryFilterProvider);
    const elastic = es;
    const tf = timefilter;
    const rootScope = $rootScope;
    queryFilter.removeAll();
    
    $scope.search = function(job) {

        var query = '{ "query" : { "bool" : { "should" : [ { "match": { "data.allocation_id" : ' + 
            job.aid +  " } } ] } } }";

        elastic.search({
            index: "cast-allocation",
            body: query
        }).then(function(body){
            // TODO this section will trigger the filter (draft 1)
            var hits = body.hits.hits;
            rootScope.$broadcast('allocationsFound', hits);
        
            var allocation = hits[0]._source;
            
            // Build the filter.
            var filter = {
              "query": {
                "multi_match": {
                  "query": allocation.data.compute_nodes.join(" "),
                  "type": "best_fields",
                  "fields": [
                    "hostname",
                    "source"
                  ],
                  "tie_breaker": 0.3,
                  "minimum_should_match": "1"
                }
              }
            };
            filter.meta = {
                disabled: false, 
                negate: false
            }
            console.log(filter);
            
            // Remove old filter.
            queryFilter.removeAll();
            // Add the filter.
            queryFilter.addFilters(filter);


            // Set the time filter.
            tf.time.mode = "absolute";
            tf.time.from = allocation.data.begin_time;
            
            if ( allocation.data.history )
                tf.time.to = allocation.data.history.end_time;
            else
                tf.time.to = Date.now;

        });
    }

});
