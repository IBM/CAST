//import { resolve } from 'path';
//import exampleRoute from './server/routes/example';

export default function (kibana) {
  return new kibana.Plugin({
    require: ['elasticsearch'],

    uiExports: {
        visTypes: [ 'plugins/cast_visualizer/register_vis' ],
        
        injectDefaultVars(server){
            const config = server.config();
            return {
                elasticsearchUrl: config.get('elasticsearch.url'),
                esShardTimeout: config.get('elasticsearch.shardTimeout'),
                esApiVersion: config.get('elasticsearch.apiVersion'),
            }
        }
    },

    init(server, options){
        server.route({
            path: '/api/cast/es_config',
            method: 'GET',
            handler(req, reply){
                const config = server.config();
                console.log(config.get('elasticsearch.tribe.url'));
                reply({
                    elasticsearchUrl: config.get('elasticsearch.url'),
                    esShardTimeout: config.get('elasticsearch.shardTimeout'),
                    esApiVersion: config.get('elasticsearch.apiVersion')
                });
            }
        });
    },
  });
};
