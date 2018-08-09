import PropTypes from 'prop-types';
import React, {
      Component,
} from 'react';

import {
EuiButton,
  EuiButtonEmpty,
  EuiFieldText,
  EuiForm,
  EuiFormRow,
  EuiModal,
  EuiModalBody,
  EuiModalFooter,
  EuiModalHeader,
  EuiModalHeaderTitle,
  EuiOverlayMask,
  EuiRange,
  EuiSwitch,


} from '@elastic/eui';


export default class CollisionModal extends Component { 
    constructor(props){
        super(props);
        console.log(this.props.responseObj);
        console.log("ModalRendering!");
        this.closeModal = this.closeModal.bind(this);
    }

    closeModal(){
        this.props.onClose(false);
    }

    render(){
        return( <EuiOverlayMask>
            <EuiModal
                onClose={this.closeModal}
                style={{ width: '800px' }}
            >
                <EuiModalHeader>
                    <EuiModalHeaderTitle >
                      Collsions Detected
                    </EuiModalHeaderTitle>
                </EuiModalHeader>


                <EuiModalBody>
            
                </EuiModalBody>

                <EuiModalFooter>
                <EuiButtonEmpty
                    onClick={this.closeModal}
                >
                    Cancel
                </EuiButtonEmpty>
            </EuiModalFooter>
            
            </EuiModal>
        </EuiOverlayMask>);
    }
    
}



CollisionModal.propTypes = {
    onClose: PropTypes.func.isRequired,
    responseObj: PropTypes.object,
    collisionFilter: PropTypes.array

}
