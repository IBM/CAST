import _ from "lodash";
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
  EuiBasicTable,
} from '@elastic/eui';


export default class CollisionModal extends Component { 
    constructor(props){
        super(props);
        this.closeModal = this.closeModal.bind(this);
        this.state = {
            columns: this.props.collisionFilter.map( (key) => {
                    return {
                        field : key,
                        name  : key,
                        truncateText : true,
                        sortable : false
                    }
                }),
            pageIndex : 0,
            pageSize  : 5,
            itemCount : this.props.responseObj.length,
            selectedItem : {},
        }
    }

        

    closeModal(){
        this.props.onClose(false);
    }

    onSelection = (selectedItems) => {
        if( selectedItems.length >0 )
        {
            this.props.onClose(this.props.responseObj[_.get(selectedItems, "[0]._itemId", -1)]);
        }
    };

    onTableChange = ( { page = {}, sort = {} }  ) =>{
        const {
          index: pageIndex,
          size: pageSize,
        } = page;

        this.setState({
            pageIndex,
            pageSize
        });
    };
    
    buildPage = ( ) => {
        var page = [];
        for (var idx = this.state.pageIndex * this.state.pageSize, count = 0; 
            idx < this.state.itemCount && count < this.state.pageSize; idx++,count++)
        {
            var record = {}
            const target=this.props.responseObj[idx]._source; // TODO MAKE THIS SMARTER!
            this.props.collisionFilter.map( (key) => {
                const value = _.get(target, key, false);
                if ( value )
                {
                    record[key] = value;
                }
            } );
            record["_itemId"] = idx;
            page.push(record);
        }
        return page;
    };

    render(){
        const itemPage = this.buildPage();

        const pagination = {
            pageIndex: this.state.pageIndex,
            pageSize: this.state.pageSize,
            totalItemCount: this.state.itemCount,
            pageSizeOptions: [5, 10, 15]
        };

        const selection = {
            selectable : () => true,
            itemId : "_itemId", 
            onSelectionChange: this.onSelection,
        };

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
                    <EuiBasicTable
                        items={itemPage}
                        itemId="id"
                        columns={this.state.columns}
                        pagination={pagination}
                        isSelectable={true}
                        selection={selection}
                        onChange={this.onTableChange}
                    />
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
    responseObj: PropTypes.array.isRequired,
    collisionFilter: PropTypes.array.isRequired

}
