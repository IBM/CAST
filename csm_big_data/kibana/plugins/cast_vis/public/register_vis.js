
import './vis.less';
import image from './images/icon.svg';

import './controllers/cast_search_vis_controller.js';
import visTestTemplate from './templates/test_vis.html';
import castVisParamsTemplate from './templates/cast_vis_params.html';

import { CATEGORY } from 'ui/vis/vis_category';
import { VisFactoryProvider } from 'ui/vis/vis_factory';
import { VisTypesRegistryProvider } from 'ui/registry/vis_types';
import { DefaultEditorSize } from 'ui/vis/editor_size';

VisTypesRegistryProvider.register(CASTVisProvider);

export default function CASTVisProvider(Private) {
    const VisFactory = Private(VisFactoryProvider);

    return VisFactory.createAngularVisualization({
        name: 'cast_search',
        title: 'CAST Search',
        isAccessible: true,
        image,
        description: 'Performs a CAST based filtering',
        category: CATEGORY.OTHER,
        visConfig: {
            defaults: {
            },
            template: visTestTemplate,
        },
        editorConfig: {
            optionsTemplate: castVisParamsTemplate,
            enableAutoApply: true,
            defaultSize: DefaultEditorSize.LARGE,
        },
        responseHandler: 'none',
        requestHandler:  'none'
    });
}
