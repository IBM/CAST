import React, { Component } from 'react';

import {
  EuiButton,
  EuiFlexGroup,
  EuiFlexItem,
  EuiFormRow,
} from '@elastic/eui';

export class CastSearchVis extends Component {

    constructor(props) {
        super(props);


        this.handleSubmit = this.handleSubmit.bind(this);
        this.handleReset  = this.handleReset.bind(this); 
    }
    

    handleSubmit() {

    }

    handleReset() {

    }

    
    renderControls() {
    }
    
    renderButtons() {
        return(
         <EuiFlexGroup>
          <EuiFlexItem grow={false}>
           <EuiFormRow>
            <EuiButton
             onClick={this.handleSubmit}
             disabled={!this.props.hasChanges()}>
                Apply Changes
            </EuiButton>
           </EuiFormRow>
          </EuiFlexItem>
         <EuiFlexGroup>
         );
    }


    render() {
        return (
            <div className="castSearchVis">
                {this.renderButtons()}
            </div>
    }
}
