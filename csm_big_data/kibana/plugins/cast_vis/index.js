//import { resolve } from 'path';
//import exampleRoute from './server/routes/example';

export default function (kibana) {
  return new kibana.Plugin({
    require: ['elasticsearch'],

    uiExports: {
      visTypes: [ 'plugins/cast_visualizer/register_vis' ]
    },
  });
};
