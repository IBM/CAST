import PropTypes from 'prop-types';
import React, { Component } from 'react';
import {getTitle} from '../../search_utils';

import {
    EuiButtonIcon,
    EuiPanel,
    EuiAccordion
} from '@elastic/eui';

export class CastSearchEditor extends Component 
{ 
    constructor(props){
        super(props);
    }

    removeSearch = () => {
        this.props.onRemove(this.props.searchIndex);
    }


    renderTopButtons() {
        return (
            <div>
              <EuiButtonIcon
                aria-label="Remove Search"
                color="danger"
                onClick={this.removeSearch}
                iconType="cross"
              />
            </div>
        );
    }

    render() {
        return(
          <div> 
           <EuiPanel grow={false} className="controlEditorPanel">
            <EuiAccordion
             id="searchEditorAccordion"
             buttonContent={getTitle(this.props.searchParams, this.props.searchIndex)}
             extraAction = { this.renderTopButtons()}
             initialIsOpen={false}
            >
            </EuiAccordion>
           </EuiPanel>
          </div>);
    }
}

CastSearchEditor.propTypes = {
    onRemove: PropTypes.func.isRequired,
    searchIndex: PropTypes.number.isRequired,
    searchParams: PropTypes.object.isRequired
};
