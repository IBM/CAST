
import './vis.less';
import image from './images/icon.svg';
import castVisParamsTemplate from './templates/cast_vis_params.html';

import { CASTSearchVisWrapper } from './vis_controller';

import { CATEGORY } from 'ui/vis/vis_category';
import { VisFactoryProvider } from 'ui/vis/vis_factory';
import { VisTypesRegistryProvider } from 'ui/registry/vis_types';
import { DefaultEditorSize } from 'ui/vis/editor_size';

VisTypesRegistryProvider.register(CASTVisProvider);

function CASTVisProvider(Private) {
    const VisFactory = Private(VisFactoryProvider);

    return VisFactory.createReactVisualization({
        name: 'cast_search',
        title: 'CAST Search',
        isAccessible: true,
        image,
        description: 'Performs a CAST based filtering',
        category: CATEGORY.OTHER,
        visConfig: {
              component: CASTSearchVisWrapper,
              defaults: {
                  fontSize: 12,
                  openLinksInNewTab: false
              },
        },
        editorConfig: {
            optionsTemplate: castVisParamsTemplate,
            enableAutoApply: true,
            defaultSize: DefaultEditorSize.LARGE,
        },
        requestHandler: 'none',
        responseHandler: 'none',
    });

}




// register the provider with the visTypes registry

export default CASTVisProvider;
