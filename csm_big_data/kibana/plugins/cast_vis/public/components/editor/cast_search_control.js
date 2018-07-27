import React { Component } from 'react';
import _ from 'lodash';
import PropTypes from 'prop-types';

import {
  EuiButton,
  EuiFlexGroup,
  EuiFlexItem,
  EuiFormRow,
  EuiPanel,
  EuiSelect,
} from '@elastic/eui';


export class CastControl extends Component {

    render(){
        return (
            <div>
                <EuiPanel class="castControl">
                </EuiPanel>
            </div>
        );
    }
}

CastControl.propTypes = {
    scope: PropTypes.object.isRequired,
    stageEditorParams: PropTypes.func.isRequired
    
};
