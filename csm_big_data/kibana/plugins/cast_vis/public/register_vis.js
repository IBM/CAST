import './vis.less';
import image from './images/icon.svg';

import './controllers/cast_search_vis_controller.js';
import { CastVisController } from './vis_controller';

import CastSearchVisWrapper from './components/vis/cast_search_vis.js';

import { CATEGORY } from 'ui/vis/vis_category';
import { VisFactoryProvider } from 'ui/vis/vis_factory';
import { VisTypesRegistryProvider } from 'ui/registry/vis_types';
import { defaultFeedbackMessage } from 'ui/vis/default_feedback_message';
import { DefaultEditorSize } from 'ui/vis/editor_size';
import { CastControlsTab } from  './components/editor/cast_controls_tab'; 

VisTypesRegistryProvider.register(CASTVisProvider);

export default function CASTVisProvider(Private) {
    const VisFactory = Private(VisFactoryProvider);
    return VisFactory.createBaseVisualization({
        name: 'cast_search',
        title: 'CAST Search',
        isAccessible: true,
        image,
        description: 'Performs a CAST based filtering',
        category: CATEGORY.OTHER,
        feedbackMessage: defaultFeedbackMessage,
        visualization: CastVisController,
        visConfig: {
            defaults: {
                searches: []
            },
        },
        editor: 'default',
        editorConfig: {
            optionTabs: [
              {
                name: 'controls',
                title: 'Controls',
                editor: CastControlsTab
              }
            ]
        },
        responseHandler: 'none',
        requestHandler:  'none'
    });
}
